#define FUSE_USE_VERSION 26
#define MAX_STATS 100

#include <libgen.h>
#include <string.h>
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <device_manager.h>
#include <stdarg.h>
#include <time.h>
#include <mntent.h>

static const char *log_file_path = "/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/fuse_debug_log.txt";
device_count = 0;

void log_debug(const char *message) {
    
    FILE *log_file = fopen(log_file_path, "a");
    
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

typedef struct {
    struct stat stat;
    char *name;       // Name of the file
    char *directory;  // Directory path where the file is located
} File;


typedef struct {
    File **files;
    size_t size;
    size_t capacity;
} FileList;

typedef struct {
    size_t size;
    char **dirs;
    size_t capacity;
    struct stat *stats;
} DirList;

static FileList file_list;
static DirList dir_list;

long calculate_file_size(const char *file_path);
long calculate_directory_size(const char *dir_path);
int find_dir(const DirList *list, const char *dir_path);


void init_file_list(FileList *list, size_t initial_capacity) {
    list->files = (File**)calloc(initial_capacity,sizeof(File *));
    list->size = 0;
    list->capacity = initial_capacity;
}

void init_dir_list(DirList *list, size_t initial_capacity) {
    list->dirs = (char **)calloc(initial_capacity, sizeof(char *));
    list->stats = (struct stat *)calloc(initial_capacity, sizeof(struct stat));
    list->size = 0;
    list->capacity = initial_capacity;
}

void free_dir_list(DirList *list) {
    for (size_t i = 0; i < list->size; i++) {
        free(list->dirs[i]);  // Free each directory path string
    }
    free(list->dirs);
    free(list->stats);
    list->dirs = NULL;
    list->stats = NULL;
    list->size = 0;
    list->capacity = 0;
}


void add_file(FileList *list, const char *name, char *directory) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->files = realloc(list->files, list->capacity * sizeof(File *));
    }

    File *new_file = (File *)malloc(sizeof(File));
    new_file->name = strdup(name);
    new_file->directory = strdup(directory);
    new_file->stat.st_size = 0;
    new_file->stat.st_mode = 0644;
    new_file->stat.st_uid = getuid();
    new_file->stat.st_gid = getgid();
    new_file->stat.st_atime = time(NULL);
    new_file->stat.st_mtime = time(NULL);
    new_file->stat.st_ctime = time(NULL);

    list->files[list->size++] = new_file;

    // Debug: Log file addition
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: Added file: %s in directory: %s", name, directory);
    log_debug(log_message);
}


const char *extract_directory_name(const char *path) {
    // Find the last slash in the path
    const char *last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        return path;  // Return the full path if no slash is found (path is a filename only)
    }

    // Return the part of the path after the last slash (the filename)
    return last_slash + 1;
}


char* slice_up_to_substring(const char* firstString, const char* secondString) {
    // Find the position of secondString in firstString
    char* position = strstr(firstString, secondString);
    
    // If secondString is not found, return NULL
    if (position == NULL) {
        return NULL;
    }
    
    // Calculate the length of the substring we want to extract
    int length = position - firstString;
    
    // Allocate memory for the new string (+1 for null terminator)
    char* result = (char*)malloc(length + 1);
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }
    
    // Copy the relevant portion of firstString to result
    strncpy(result, firstString, length);
    result[length] = '\0'; // Null-terminate the string
    
    return result;
}


void get_parent_directory(const char *path, char *parent) {
    char log_message[512];

    // Safely copy the path into the parent buffer
    snprintf(parent, 512, "%s", path);  // Using snprintf to avoid buffer overflow

    // Find the last slash in the path
    char *last_slash = strrchr(parent, '/');

    // If the last slash is at the beginning (root directory), set the parent to "/"
    if (last_slash == parent) {
        snprintf(parent, 512, "/");
        return;
    }

    // Otherwise, slice off the filename to get the parent directory
    if (last_slash != NULL) {
        *last_slash = '\0';  // Replace the last slash with the null terminator
    }

    snprintf(log_message, sizeof(log_message), "DEBUG: Parent directory is %s", parent);
    log_debug(log_message);
}


int is_directory(const char *path) {
    // Use find_dir to determine if the path is a directory
    return find_dir(&dir_list, path) != -1;
}

int is_file(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return 0; // Error
    return S_ISREG(statbuf.st_mode);
}

File *find_file(FileList *list, const char *name, const char *directory) {
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->files[i]->name, name) == 0 && strcmp(list->files[i]->directory, directory) == 0) {
            // Debug: Found the file
            char log_message[512];
            snprintf(log_message, sizeof(log_message), "DEBUG: File found: %s in directory: %s", name, directory);
            log_debug(log_message);
            return list->files[i];
        }
    }
    return NULL;
}




void free_file_list(FileList *list) {
    for (size_t i = 0; i < list->size; i++) {
        free(list->files[i]->name);
        free(list->files[i]->directory);
        free(list->files[i]);
    }
    free(list->files);
    list->files = NULL;
    list->size = 0;
    list->capacity = 0;
}


int find_dir(const DirList *list, const char *dir_path) {
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->dirs[i], dir_path) == 0) {
            // Debug: Found the directory
            char log_message[512];
            snprintf(log_message, sizeof(log_message), "DEBUG: Directory found: %s", dir_path);
            log_debug(log_message);
            return i;
        }
    }
    return -1;
}




long calculate_file_size(const char *file_path) {
    FILE *file = fopen(file_path, "rb");  // Open the file in binary mode
    if (!file) {
        perror("Error opening file");
        return -1;  // Indicate an error
    }

    fseek(file, 0, SEEK_END);  // Move to the end of the file
    long size = ftell(file);   // Get the current file pointer position (size in bytes)
    fclose(file);

    return size;
}

long calculate_directory_size(const char *dir_path) {
    long total_size = 0;

    // Iterate through the file list to sum the size of files in the directory
    for (size_t i = 0; i < file_list.size; i++) {
        if (strcmp(file_list.files[i]->directory, dir_path) == 0) {
            total_size += file_list.files[i]->stat.st_size;
        }
    }

    return total_size;
}


void add_dir(DirList *dir_list, const char *dir_path) {
    // Check if the directory already exists
    for (size_t i = 0; i < dir_list->size; i++) {
        if (strcmp(dir_list->dirs[i], dir_path) == 0) {
            return;  // Directory already exists
        }
    }

    // Resize if needed
    if (dir_list->size == dir_list->capacity) {
        dir_list->capacity *= 2;
        dir_list->dirs = realloc(dir_list->dirs, dir_list->capacity * sizeof(char *));
        dir_list->stats = realloc(dir_list->stats, dir_list->capacity * sizeof(struct stat));
        if (!dir_list->dirs || !dir_list->stats) {
            perror("Failed to resize directory list");
            exit(EXIT_FAILURE);
        }
    }

    // Add the directory path
    dir_list->dirs[dir_list->size] = strdup(dir_path);

    // Populate stats for the directory
    dir_list->stats[dir_list->size].st_size = 0; // You can use a function if needed
    dir_list->stats[dir_list->size].st_mode = S_IFDIR | 0755;  // Default permissions
    dir_list->stats[dir_list->size].st_uid = getuid();  // Owner user ID
    dir_list->stats[dir_list->size].st_gid = getgid();  // Owner group ID
    dir_list->stats[dir_list->size].st_atime = time(NULL);
    dir_list->stats[dir_list->size].st_mtime = time(NULL);
    dir_list->stats[dir_list->size].st_ctime = time(NULL);

    dir_list->size++;
}



//=================================================================
//=================================================================



static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));  // Clear the stat structure
    char parent_dir[1024];
    char log_message[512];

    // Handle the root directory
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0775;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = time(NULL);
        stbuf->st_ctime = time(NULL);
        return 0;
    }

    // Check if the path is a directory
    int dir_index = find_dir(&dir_list, path);
    if (dir_index != -1) {
        stbuf->st_mode = S_IFDIR | 0755;  // Set as a directory
        stbuf->st_nlink = 2;
        stbuf->st_size = calculate_directory_size(path);  // Directory size
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = dir_list.stats[dir_index].st_atime;
        stbuf->st_mtime = dir_list.stats[dir_index].st_mtime;
        stbuf->st_ctime = dir_list.stats[dir_index].st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for directory: %s", path);
        log_debug(log_message);
        return 0;
    }

    // Check if the path is a file
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    File *file = find_file(&file_list, file_name, parent_dir);

    if (file) {
        stbuf->st_mode = S_IFREG | 0644;  // Set as a regular file
        stbuf->st_size = calculate_file_size(path);
        stbuf->st_nlink = 1;
        stbuf->st_size = file->stat.st_size;  // File size
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = file->stat.st_atime;
        stbuf->st_mtime = file->stat.st_mtime;
        stbuf->st_ctime = file->stat.st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for file: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return 0;
    }

    // If not found, return an error
    snprintf(log_message, sizeof(log_message), "DEBUG: getattr failed, path not found: %s", path);
    log_debug(log_message);
    return -ENOENT;
}



static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: readdir_callback called with path = %s", path);
    log_debug(log_message);

    // Add default entries for the current and parent directories
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // List directories in the current path based on dir_list structure
    for (size_t i = 0; i < dir_list.size; i++) {
        // Check if the current directory is a subdirectory of the given path
        char parent_dir[1024];
        get_parent_directory(dir_list.dirs[i], parent_dir);
        snprintf(log_message, sizeof(log_message), "DEBUG: Parent_dir: %s and path: %s", parent_dir,path);
        log_debug(log_message);

        if (strcmp(parent_dir, path) == 0) {
            const char *base_name = extract_directory_name(dir_list.dirs[i]);
            filler(buf, base_name, NULL, 0);
            snprintf(log_message, sizeof(log_message), "DEBUG: Listed directory: %s", base_name);
            log_debug(log_message);
        }
    }

    // List files in the current directory based on file_list structure
    for (size_t i = 0; i < file_list.size; i++) {
        if (strcmp(file_list.files[i]->directory, path) == 0) {
            filler(buf, file_list.files[i]->name, NULL, 0);

            snprintf(log_message, sizeof(log_message), "DEBUG: Listed file: %s in directory: %s", 
                     file_list.files[i]->name, file_list.files[i]->directory);
            log_debug(log_message);
        }
    }

    return 0;
}

static void* init_callback(struct fuse_conn_info *conn) {
    // Clear the log file by opening it in write mode
    FILE *log_file = fopen(log_file_path, "w");
    if (log_file) {
        fclose(log_file);  // Just open and close to clear the file content
    } else {
        // Handle error if log file can't be cleared
        perror("Error clearing log file");
    }
    return NULL;
    // You can optionally log that the filesystem was mounted successfully
    log_debug("Filesystem mounted and log file cleared.");
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    char parent_dir[1024];
    char log_message[512];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);

    if (find_file(&file_list, file_name, parent_dir) != NULL){
        snprintf(log_message, sizeof(log_message), "DEBUG: File opened successfully: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return 0;  // Success
    }
    snprintf(log_message, sizeof(log_message), "DEBUG: File not opened");
    log_debug(log_message);
    return -ENOENT;
}

static int utimens_callback(const char *path, const struct timespec tv[2]) {
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    char log_message[512];

    // Check if the target is a directory
    int dir_index = find_dir(&dir_list, path);
    if (dir_index != -1) {
        // Update directory timestamps
        dir_list.stats[dir_index].st_atime = tv ? tv[0].tv_sec : time(NULL);
        dir_list.stats[dir_index].st_mtime = tv ? tv[1].tv_sec : time(NULL);

        snprintf(log_message, sizeof(log_message), "DEBUG: Updated timestamps for directory: %s", path);
        log_debug(log_message);
        return 0;
    }

    // Check if the target is a file
    File *file = find_file(&file_list, file_name, parent_dir);
    if (file) {
        // Update file timestamps
        file->stat.st_atime = tv ? tv[0].tv_sec : time(NULL);
        file->stat.st_mtime = tv ? tv[1].tv_sec : time(NULL);

        int parent_dir_index = find_dir(&dir_list, parent_dir);
        if (parent_dir_index != -1) {
            // Set the parent directory's modification time to the current time
            dir_list.stats[parent_dir_index].st_mtime = time(NULL);

            snprintf(log_message, sizeof(log_message), "DEBUG: Updated modification time for parent directory: %s", parent_dir);
            log_debug(log_message);
        } else {
            snprintf(log_message, sizeof(log_message), "DEBUG: Parent directory %s not found for updating timestamps", parent_dir);
            log_debug(log_message);
        }
        

        snprintf(log_message, sizeof(log_message), "DEBUG: Updated timestamps for file: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return 0;
    }

    // If neither file nor directory is found, return an error
    snprintf(log_message, sizeof(log_message), "DEBUG: Timestamps update failed: %s not found", path);
    log_debug(log_message);
    return -ENOENT;
}


static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;  // Suppress unused variable warning

    // Extract the parent directory and file name from the path
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);

    // Check if the file already exists in the FileList
    if (find_file(&file_list, file_name, parent_dir) != NULL) {
        return -EEXIST;  // File already exists
    }

    // Check if the parent directory exists in DirList
    if (find_dir(&dir_list, parent_dir) == -1) {
        return -ENOENT;  // Parent directory does not exist
    }

    // Add the new file to the file list
    add_file(&file_list, file_name, parent_dir);

    // Optionally, set additional attributes if required (e.g., permissions)
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: File created successfully: %s in directory: %s", file_name, parent_dir);
    log_debug(log_message);

    struct timespec ts[2];
    clock_gettime(CLOCK_REALTIME, &ts[0]);  // Current time for atime
    ts[1] = ts[0];  // Same time for mtime
    utimens_callback(path, ts);

    return 0;  // Success
}


/*static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

  if (strcmp(path, filepath) == 0) {
    size_t len = strlen(filecontent);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  return -ENOENT;
}*/

static int mkdir_callback(const char *path, mode_t permission_bits) {
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: mkdir_callback called with path = %s, permissions = %o", path, permission_bits);
    log_debug(log_message);

    // Check if the directory already exists using find_dir()
    if (find_dir(&dir_list, path) != -1) {
        snprintf(log_message, sizeof(log_message), "DEBUG: Directory already exists: %s", path);
        log_debug(log_message);
        return -EEXIST;  // Return error if it already exists
    }

    // Add the directory to our custom list
    add_dir(&dir_list, path);

    snprintf(log_message, sizeof(log_message), "DEBUG: Directory added successfully: %s", path);
    log_debug(log_message);
    return 0;  // Success
}

void remove_dir(DirList *list, size_t index) {
    if (index >= list->size) {
        return;  // Invalid index, nothing to remove
    }

    // Free the memory allocated for the directory path
    free(list->dirs[index]);

    // Shift remaining elements to fill the gap
    for (size_t i = index; i < list->size - 1; i++) {
        list->dirs[i] = list->dirs[i + 1];
        list->stats[i] = list->stats[i + 1];
    }

    // Decrement the size of the list
    list->size--;
}

static int rmdir_callback(const char *path) {
    // Check if the directory exists using find_dir()
    int dir_index = find_dir(&dir_list, path);
    if (dir_index == -1) {
        return -ENOENT;  // Directory not found
    }

    // Perform the deletion from DirList
    remove_dir(&dir_list, dir_index);  // Hypothetical function to remove the directory
    return 0;  // Success
}



static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .create = create_callback,
  //.read = read_callback,
  .readdir = readdir_callback,
  .init = init_callback,
  .mkdir = mkdir_callback,
  .utimens = utimens_callback,
  .rmdir = rmdir_callback,
};

int main(int argc, char *argv[])
{

  init_file_list(&file_list,10);
  init_dir_list(&dir_list,10);
  int result = fuse_main(argc, argv, &fuse_example_operations, NULL);

  // Clean up resources before exiting
  free_file_list(&file_list);
  free_dir_list(&dir_list);

  return result;

}
