#define FUSE_USE_VERSION 26
#define MAX_STATS 100

#include <libgen.h>
#include <regex.h>
#include <string.h>
#include <ctype.h>
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include "device_manager.h"
#include <stdarg.h>
#include <time.h>
#include<json-c/json.h>
#include <mntent.h>

static const char *log_file_path = "/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/fuse_debug_log.txt";
const char *json_path = "/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/json_test_example.json";

extern void log_debug(const char *message) {
    
    FILE *log_file = fopen(log_file_path, "a");
    
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Structure to hold parsed directory name components
typedef struct {
    char *name;
    int serial_number;
    char *imei;
    char *model;
} ParsedInput;

typedef struct {
    struct stat stat;  // Metadata
    char *name;        // Name of the file
    char *directory;   // Directory path where the file is located
    char *data;        // Pointer to the file's content
    size_t capacity;   // Allocated size of the data buffer
    char read_type[20];
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
    new_file->data = NULL;       // Initialize data to NULL
    new_file->capacity = 0;     // No capacity allocated initially

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
    char log_message[512];
    for (size_t i = 0; i < list->size; i++) {
        snprintf(log_message, sizeof(log_message), "DEBUG: Directory: %s, and dir_path: %s", list->dirs[i],dir_path);
        log_debug(log_message);
        if (strcmp(list->dirs[i], dir_path) == 0) {
            // Debug: Found the directory
            snprintf(log_message, sizeof(log_message), "DEBUG: Directory found: %s", dir_path);
            log_debug(log_message);
            return i;
        }
    }
    return -1;
}

long calculate_file_size(const char *file_path) {
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(file_path, parent_dir);
    const char *file_name = extract_directory_name(file_path);

    // Search for the file in the FileList
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -1;  // Indicate an error
    }

    // Return the size from the file's metadata
    snprintf(log_message, sizeof(log_message), "INFO: Calculated size for file: %s is %ld bytes.", file_name, file->stat.st_size);
    log_debug(log_message);

    return file->stat.st_size;
}



long calculate_directory_size(const char *dir_path) {
    long total_size = 0;
    log_debug("inside calc dir size.");
    // Iterate through the file list to sum the size of files in the directory
    for (size_t i = 0; i < file_list.size; i++) {
        if (strcmp(file_list.files[i]->directory, dir_path) == 0) {
            total_size += file_list.files[i]->stat.st_size;
            log_debug("inside calc dir size.");
        }
    }
    return total_size;
}

void generate_random_string(char *random_string, size_t length) {
    srand((unsigned int)time(NULL));
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.+-/*?!@#$%^|&";
    size_t charset_size = strlen(charset);

    for (size_t i = 0; i < length; i++) {
        int key = rand() % charset_size; // Random index in charset
        random_string[i] = charset[key];
    }
    random_string[length] = '\0'; // Null-terminate the string
}

void modify_path(const char *path, const char *directory_name, char *new_path) {
    // Find the position of the last slash in the path to identify the directory part
    const char *last_slash = strrchr(path, '/');

    // Find the position of the first dot in the directory_name
    const char *dot_pos = strchr(directory_name, '.');

    // Extract the part of directory_name before the first dot
    char modified_directory[256];  // Assuming the directory name is not too long
    if (dot_pos != NULL) {
        size_t len = dot_pos - directory_name;
        strncpy(modified_directory, directory_name, len);
        modified_directory[len] = '\0';  // Null-terminate the string
    } else {
        // If no dot, take the whole directory_name
        strcpy(modified_directory, directory_name);
    }

    // Copy the original path up to the last slash, then append the modified directory name
    size_t path_length = last_slash - path + 1;  // Include the last slash
    strncpy(new_path, path, path_length);
    new_path[path_length] = '\0';  // Null-terminate after the path

    // Append the modified directory_name
    strcat(new_path, modified_directory);
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

void extract_model(const char *input, char *output) {

    // Copy the input string to a temporary buffer to tokenize
    char temp[256];
    strncpy(temp, input, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';  // Ensure null-termination

    // Tokenize the string using '.' as the delimiter
    char *token1 = strtok(temp, ".");
    char *token2 = strtok(NULL, ".");
    char log_message[512];
    snprintf(log_message,sizeof(log_message),"%s and %s tokens.",token1,token2);
    log_debug(log_message);
    strncpy(output, token2, strlen(token2) + 1);
}

void get_substring_up_to_char(const char *input, char *output, char delimiter) {

    // Find the last occurrence of the delimiter in the input string
    const char *delimiter_pos = strrchr(input, delimiter);
    
    if (delimiter_pos != NULL) {
        // Copy the part of the string up to the delimiter
        size_t length = delimiter_pos - input;
        strncpy(output, input, length);
        output[length] = '\0';  // Null-terminate the substring
    } else {
        // If delimiter is not found, copy the whole string
        strcpy(output, input);
    }
}

void modify_path_to_remove_serial(const char *input_path, char *output_path) {
    char log_message[256];
    get_substring_up_to_char(input_path,output_path,'.');
    snprintf(log_message,sizeof(log_message),"Output path in modify path to remove subs: %s",output_path);
    log_debug(log_message);
}

void count_dots(const char* string,int* return_result){
    for(int i = 0;i<strlen(string);i++){
        if(string[i] == '.')    (*return_result)++;
    }
}

static int getattr_callback(const char *path, struct stat *stbuf) {
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: Getattr callback called with path: %s.", path);
    log_debug(log_message);
    memset(stbuf, 0, sizeof(struct stat));  // Clear the stat structure
    int dot_counter = 0;

    count_dots(extract_directory_name(path),&dot_counter);

    char parent_dir[1024];

    // Handle the root directory
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0775;
        stbuf->st_nlink = 2;
        stbuf->st_size = calculate_directory_size(path);
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = dir_list.stats[0].st_atime;
        stbuf->st_mtime = dir_list.stats[0].st_mtime;
        stbuf->st_ctime = dir_list.stats[0].st_ctime;
        add_dir(&dir_list,"/");
        return 0;
    }
    if(!strcmp(extract_directory_name(path),"GYRO") || !strcmp(extract_directory_name(path),"GPS") || !strcmp(extract_directory_name(path),"IMEI")){
        stbuf->st_mode = S_IFREG | 0444;  // Set as a regular file
        stbuf->st_size = 0;
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = dir_list.stats[0].st_atime;
        stbuf->st_mtime = dir_list.stats[0].st_mtime;
        stbuf->st_ctime = dir_list.stats[0].st_ctime;
        return 0;
    }
    char* new_path = strdup(path);
    const char *dir_name = extract_directory_name(path);
    
    // Check if the path is a directory
    if(dot_counter == 2){
        modify_path(path,dir_name,new_path);
    }
    
    int dir_index = find_dir(&dir_list, new_path);

    if (dir_index != -1) {
        stbuf->st_mode = S_IFDIR | 0755;  // Set as a directory
        stbuf->st_nlink = 2;
        stbuf->st_size = calculate_directory_size(new_path);  // Directory size
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();   
        stbuf->st_atime = dir_list.stats[dir_index].st_atime;
        stbuf->st_mtime = dir_list.stats[dir_index].st_mtime;
        stbuf->st_ctime = dir_list.stats[dir_index].st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for directory: %s, its size is: %d.", new_path,stbuf->st_size);
        log_debug(log_message);
        return 0;
    }

    // Check if the path is a file
    char* secondary_path = strdup(path);
    if(dot_counter == 2){
        modify_path_to_remove_serial(path,secondary_path);
    }
    get_parent_directory(secondary_path, parent_dir);

    const char *file_name = extract_directory_name(secondary_path);
    File *file = find_file(&file_list, file_name, parent_dir);

    if (file) { 
        char* model = strrchr(file_name,'.');
        if(!strcmp(model+1,"ACTUATOR")) stbuf->st_mode = __S_IFREG | 0222;
        else if(!strcmp(model+1,"SENSOR")) stbuf->st_mode = __S_IFREG | 0444;
        else stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_size = calculate_file_size(secondary_path);
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = file->stat.st_atime;
        stbuf->st_mtime = file->stat.st_mtime;
        stbuf->st_ctime = file->stat.st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for file: %s in directory: %s. Its size is: %d.", file_name, parent_dir,stbuf->st_size);
        log_debug(log_message);
        return 0;
    }

    // If not found, return an error
    snprintf(log_message, sizeof(log_message), "DEBUG: getattr failed, new_path not found: %s nor secondary_path has been found %s.", new_path,secondary_path);
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
    if(strcmp(path, "/") != 0){
        filler(buf, "GYRO", NULL, 0);
        filler(buf, "GPS", NULL, 0);
        filler(buf, "IMEI", NULL, 0); 
    }
    snprintf(log_message, sizeof(log_message), "DEBUG: Reading after main fillers.");
    log_debug(log_message);

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
    }
    FILE *json_file = fopen(json_path,"w");
    if (json_file) {
        fclose(json_file);  // Just open and close to clear the file content
    }
    // You can optionally log that the filesystem was mounted successfully
    log_debug("Filesystem mounted and log file cleared && json file cleared.");
    return NULL;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    log_debug("Inside open callback.");
    char parent_dir[1024];
    char log_message[512];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    if(!strcmp(file_name,"GPS") || !strcmp(file_name,"IMEI") || !strcmp(file_name,"GYRO")){
        log_debug("Special file detected.");
        return 0;
    }
    if (find_file(&file_list, file_name, parent_dir) != NULL){
        snprintf(log_message, sizeof(log_message), "DEBUG: File opened successfully: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return 0;  // Success
    }
    snprintf(log_message, sizeof(log_message), "DEBUG: File not opened. File name is: %s.",file_name);
    log_debug(log_message);
    return -ENOENT;
}

static int utimens_callback(const char *path, const struct timespec tv[2]) {
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: Utimens callback called with %s as path.", path);
    char* secondary_path = (char*)calloc(100,sizeof(char));
    modify_path_to_remove_serial(path,secondary_path);
    log_debug(log_message);
    char parent_dir[1024];
    get_parent_directory(secondary_path, parent_dir);
    const char *file_name = extract_directory_name(secondary_path);

    // Check if the target is a directory
    int dir_index = find_dir(&dir_list, secondary_path);
    if (dir_index != -1) {
        // Update directory timestamps
        dir_list.stats[dir_index].st_atime = tv ? tv[0].tv_sec : time(NULL);
        dir_list.stats[dir_index].st_mtime = tv ? tv[1].tv_sec : time(NULL);

        snprintf(log_message, sizeof(log_message), "DEBUG: Updated timestamps for directory: %s", secondary_path);
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
    snprintf(log_message, sizeof(log_message), "DEBUG: Timestamps update failed: %s not found", secondary_path);
    log_debug(log_message);
    return -ENOENT;
}


int check_restrictions(const char *input, const char *parent_directory, ParsedInput* parsed_input) {
    char log_message[512];
    regex_t regex;
    const char *pattern = "^[a-zA-Z0-9_]+\\.[a-zA-Z0-9_-]+\\.[0-9]+$";

    // Compile the regex
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        log_debug("ERROR: Failed to compile regex.");
        return -EINVAL;
    }

    // Check if the directory name matches the expected format
    if (regexec(&regex, input, 0, NULL, 0) != 0) {
        log_debug("ERROR: File name format is invalid. Expected format: name.model.serial_number");
        regfree(&regex);
        return -EINVAL;
    }
    regfree(&regex);

    if (input == NULL || parent_directory == NULL) {
        snprintf(log_message,sizeof(log_message),"At least one of the parameters is null.");
        log_debug(log_message);
        return 0;
    }

    char *copy = strdup(input);
    if (!copy) {
        snprintf(log_message,sizeof(log_message),"ERROR: Memory allocation failed for input copy.");
        log_debug(log_message);
        return 0;
    }

    char *string1 = strtok(copy, ".");
    char *string2 = strtok(NULL, ".");
    char *string3 = strtok(NULL, ".");

    if (string1 == NULL || string2 == NULL || string3 == NULL || strtok(NULL, ".") != NULL) {
        snprintf(log_message,sizeof(log_message),"ERROR: Input format is invalid. Expected format: string1.string2.string3.");
        log_debug(log_message);
        free(copy);
        return 0;
    }

    // Validate string2 as a model
    if (!is_valid_model(string2, FILE_TYPE)) {
        snprintf(log_message,sizeof(log_message),"ERROR: Invalid model specified, %s.",string2);
        log_debug(log_message);
        free(copy);
        return 0;
    }

    // Validate parent directory is not the root
    if (strcmp(parent_directory, "/") == 0) {
        snprintf(log_message,sizeof(log_message),"ERROR: Files cannot be created in the root directory.");
        log_debug(log_message);
        free(copy);
        return 0;
    }

    // Ensure string3 is a numeric serial number
    for (size_t i = 0; i < strlen(string3); i++) {
        if (!isdigit(string3[i])) {
            snprintf(log_message,sizeof(log_message),"ERROR: Serial number must be numeric: %s\n", string3);
            log_debug(log_message);
            free(copy);
            return 0;
        }
    }
    parsed_input->name = strdup(string1);
    parsed_input->model = strdup(string2);
    parsed_input->serial_number = atoi(string3);
    parsed_input->imei = "";
    snprintf(log_message,sizeof(log_message),"INFO: %s, %s, %s, %s.", string1,string2,string3,parsed_input->imei);
    log_debug(log_message);
    free(copy);
    return 1;
}

static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;  // Suppress unused variable warning
    char log_message[512];
    time_t registration_date = time(NULL);
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    if(!strcmp(file_name,"GYRO") || !strcmp(file_name,"IMEI") || !strcmp(file_name,"GPS")){
        snprintf(log_message, sizeof(log_message), "DEBUG: Creating %s file.", file_name);
        log_debug(log_message);
        return 0;
    }
    ParsedInput parsed_input;

    if(!check_restrictions(file_name,parent_dir,&parsed_input)){
        snprintf(log_message,sizeof(log_message),"ERROR: Restrictions not set.");
        log_debug(log_message);
        return -EXIT_FAILURE;
    }

    char real_path[256];

    modify_path_to_remove_serial(path,real_path);

    const char* real_file_name = extract_directory_name(real_path);

    snprintf(log_message, sizeof(log_message), "DEBUG: Real path: %s", real_path);
    log_debug(log_message);

    // Check if the file already exists in the FileList
    if (find_file(&file_list, real_file_name, parent_dir) != NULL) {
        return -EEXIST;  // File already exists
    }

    // Check if the parent directory exists in DirList
    if (find_dir(&dir_list, parent_dir) == -1) {
        return -ENOENT;  // Parent directory does not exist
    }
    DeviceEntry *device = create_and_add_device_entry(
        parsed_input.name, parsed_input.model, parsed_input.serial_number, registration_date, parsed_input.imei, FILE_TYPE);
    // Add the new file to the file list
    if(device == NULL){
        snprintf(log_message, sizeof(log_message), "DEBUG: Device is null.");
        log_debug(log_message);
    }
    add_file(&file_list, real_file_name, parent_dir);
    add_device_to_json(device, json_path, extract_directory_name(parent_dir));

    // Optionally, set additional attributes if required (e.g., permissions)
    snprintf(log_message, sizeof(log_message), "DEBUG: File created successfully: %s in directory: %s", real_file_name, parent_dir);
    log_debug(log_message);

    struct timespec ts[2];
    clock_gettime(CLOCK_REALTIME, &ts[0]);  // Current time for atime
    ts[1] = ts[0];  // Same time for mtime
    //utimens_callback(real_path, ts);

    return 0;  // Success
}


static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

    log_debug("Inside read callback function.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    if(!strcmp(file_name,"GPS")){
        snprintf(log_message, sizeof(log_message), "%d %d", rand(), rand());
        log_debug(log_message);
        return size;
    }
    else if(!strcmp(file_name,"GYRO")){
        snprintf(log_message, sizeof(log_message),"%d %d %d",rand(),rand());
        log_debug(log_message);
        return size;
    }
    else if(!strcmp(file_name,"IMEI")){
        const char* dev_imei = find_imei(extract_directory_name(parent_dir),json_path);
        snprintf(log_message, sizeof(log_message),"Device IMEI: %s.",dev_imei);
        log_debug(log_message);
        return size;
    }
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; // File not found
    }
    if(!strcmp(file->read_type,"data")){
        char rand_seq[8];
        generate_random_string(rand_seq,8);
        snprintf(log_message, sizeof(log_message),"RAND SEQ: %s.",rand_seq);
        log_debug(log_message);
        return size;
    }
    if(!strcmp(file->read_type,"info")){
        struct json_object *device = find_device(file_name, json_path);
        struct json_object *name_obj;
        struct json_object *ser_num;
        struct json_object *reg_date;
        struct json_object *sys_id;
        struct json_object *model;
        if (json_object_object_get_ex(device, "Name", &name_obj)){
            snprintf(log_message,sizeof(log_message),"Device Name: %s\n", json_object_get_string(name_obj));
        }
        if (json_object_object_get_ex(device, "Model", &model)){
            snprintf(log_message,sizeof(log_message),"Device model: %s\n", json_object_get_string(model));
        }
        if(json_object_object_get_ex(device, "SerialNumber", &ser_num)){
            snprintf(log_message,sizeof(log_message),"Device serial num: %s\n", json_object_get_string(ser_num));
        }
        if(json_object_object_get_ex(device, "RegistrationDate", &reg_date)){
            snprintf(log_message,sizeof(log_message),"Device reg date: %s\n", json_object_get_string(reg_date));
        }
        /*if(json_object_object_get_ex(device, "System id", &sys_id)){
            snprintf(log_message,sizeof(log_message),"Device sys id: %s\n", json_object_get_string(sys_id));
        }*/
        log_debug(log_message);
        return size;
    }
    

    return size;
}

static int validate_and_parse_mkdir_input(const char *dir_name, ParsedInput *parsed) {
    regex_t regex;
    const char *pattern = "^[a-zA-Z0-9_]+\\.[a-zA-Z0-9_-]+\\.[0-9]+$";

    // Compile the regex
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        log_debug("ERROR: Failed to compile regex.");
        return -EINVAL;
    }

    // Check if the directory name matches the expected format
    if (regexec(&regex, dir_name, 0, NULL, 0) != 0) {
        log_debug("ERROR: Directory name format is invalid. Expected format: name.serial_number.imei");
        regfree(&regex);
        return -EINVAL;
    }
    regfree(&regex);

    // Parse the directory name into components
    char *name = strtok(strdup(dir_name), ".");
    char *serial_number_str = strtok(NULL, ".");
    char *imei_str = strtok(NULL, ".");

    // Validate and parse serial_number and IMEI
    if (!serial_number_str || !imei_str) {
        log_debug("ERROR: Invalid directory name components.");
        free(name);
        return -EINVAL;
    }

    for (size_t i = 0; i < strlen(serial_number_str); i++) {
        if (!isdigit(serial_number_str[i])) {
            log_debug("ERROR: Serial number must be numeric.");
            free(name);
            return -EINVAL;
        }
    }

    for (size_t i = 0; i < strlen(imei_str); i++) {
        if (!isdigit(imei_str[i])) {
            log_debug("ERROR: IMEI must be numeric.");
            free(name);
            return -EINVAL;
        }
    }

    // Populate the parsed structure
    parsed->name = name;
    parsed->serial_number = atoi(serial_number_str);
    parsed->imei = imei_str;
    parsed->model = NULL;

    return 0;  // Success
}


static int mkdir_callback(const char *path, mode_t permission_bits) {
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: mkdir_callback called with path = %s, permissions = %o", path, permission_bits);
    log_debug(log_message);

    // Ensure the directory is being created in the root directory
    if (strchr(path + 1, '/') != NULL) {  // Check if there is more than one '/' in the path
        log_debug("ERROR: Directories can only be created in the root.");
        return -EPERM;  // Operation not permitted
    }

    // Extract the directory name from the path
    const char *dir_name = extract_directory_name(path);
    
    char new_path[512];

    modify_path(path, dir_name, new_path);


    // Validate and parse the directory name
    ParsedInput parsed;
    int validation_result = validate_and_parse_mkdir_input(dir_name, &parsed);
    if (validation_result != 0) {
        return validation_result;  // Return error code if validation fails
    }

    // Create the directory
    if (find_dir(&dir_list, new_path) != -1) {
        log_debug("ERROR: Directory already exists.");
        free(parsed.name);
        return -EEXIST;  // Directory already exists
    }
    add_dir(&dir_list, new_path);

    // Create and add the device entry
    time_t registration_date = time(NULL);
    DeviceEntry *device = create_and_add_device_entry(
        parsed.name, "TTConnectWave", parsed.serial_number, registration_date, parsed.imei, FOLDER_TYPE);

    if (!device) {
        log_debug("ERROR: Failed to create device entry.");
        free(parsed.name);
        return -ENOMEM;  // Out of memory
    }

    // Add the device to JSON
    const char *parent_name = "/";  // Assuming parent_name is the root directory
    add_device_to_json(device, json_path, parent_name);

    mode_t must_have_files_permissions = S_IRGRP | S_IROTH | S_IRUSR;
    void* unused_variable = NULL;
    char* copied_path = strdup(path);
    char* copied_path_2 = strdup(path);
    char* copied_path_3 = strdup(path);
    create_callback(strcat(copied_path,"/GYRO"),must_have_files_permissions,unused_variable);
    create_callback(strcat(copied_path_2,"/GPS"),must_have_files_permissions,unused_variable);
    create_callback(strcat(copied_path_3,"/IMEI"),must_have_files_permissions,unused_variable);

    snprintf(log_message, sizeof(log_message), "INFO: Directory %s created successfully and device added to JSON.", new_path);
    log_debug(log_message);

    // Free allocated memory
    free(parsed.name);

    return 0;
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
    remove_dir(&dir_list, dir_index);

    remove_device_from_json(extract_directory_name(path),json_path);
    return 0;  // Success
}

void remove_file(FileList *file_list, const char *path) {
    char log_message[512];

    // Extract the parent directory
    char parent_dir[512];
    get_parent_directory(path, parent_dir);

    // Extract the file name
    const char *file_name = strrchr(path, '/');
    if (!file_name) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid path format: %s", path);
        log_debug(log_message);
        return;
    }
    file_name++;  // Move past the slash to get the file name

    // Use find_file to locate the file in the list
    File *file = find_file(file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return;
    }

    // Log the removal
    snprintf(log_message, sizeof(log_message), "INFO: Removing file: %s from directory: %s", file_name, parent_dir);
    log_debug(log_message);

    // Remove the file from the list
    for (size_t i = 0; i < file_list->size; i++) {
        if (file_list->files[i] == file) {
            // Free the file's resources
            free(file->name);
            free(file->directory);
            free(file);

            // Shift remaining files to fill the gap
            for (size_t j = i; j < file_list->size - 1; j++) {
                file_list->files[j] = file_list->files[j + 1];
            }

            // Decrease the file_list size
            file_list->size--;

            // Log success
            snprintf(log_message, sizeof(log_message), "INFO: File successfully removed: %s", file_name);
            log_debug(log_message);
            return;
        }
    }
}


static int unlink_callback(const char *path) {
    char log_message[512];

    snprintf(log_message, sizeof(log_message), "DEBUG: unlink_callback called with path = %s", path);
    log_debug(log_message);

    // Check if the file exists in the FileList
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    char *file_name = extract_directory_name(path);

    if (find_file(&file_list, file_name, parent_dir) == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT;  // File not found
    }

    remove_file(&file_list, path);

    char* real_file_name = (char*)calloc(20,sizeof(char));
    get_substring_up_to_char(file_name,real_file_name,'.');    

    remove_device_from_json(real_file_name,json_path);

    snprintf(log_message, sizeof(log_message), "INFO: File successfully unlinked: %s", path);
    log_debug(log_message);
    free(real_file_name);
    return 0;  // Success
}

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    log_debug("Inside write callback function.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    size_t required_capacity = offset + size;
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; // File not found
    }
    snprintf(log_message,sizeof(log_message),"%s, %d",buf,strlen(buf));
    log_debug(log_message);
    char* dev_model = strrchr(file_name,'.') + 1;
    if(!strcmp(dev_model,"ACTUATOR")){
        log_debug("Inside strcmp statement for actuator.");
        strcpy(file->read_type,buf);
        snprintf(log_message,sizeof(log_message),"[%s] : %s",file_name,buf);
        log_debug(log_message);
        file->stat.st_mtime = time(NULL); // Update modification time
        return size;
    }
    else if(!strcmp(buf,"data\n")){
        log_debug("inside strcmp statement for data");
        strcpy(file->read_type,"data");
        snprintf(log_message,sizeof(log_message),"[%s] : data",file_name);
        log_debug(log_message);
        file->stat.st_mtime = time(NULL); // Update modification time
        return size;
    } 
    else if(!strcmp(buf,"info\n")){
        log_debug("inside strcmp statement for info");
        strcpy(file->read_type,"info");
        log_debug("inside strcmp statement for info, after strcpy.");
        snprintf(log_message,sizeof(log_message),"[%s] : info",file_name);
        log_debug(log_message);
        file->stat.st_mtime = time(NULL); // Update modification time
        return size;
    }
    else{
        log_debug("ERROR: invalid writing.");
        return -EPERM;
    }
}

static int truncate_callback(const char *path, off_t size) {
    log_debug("Inside the truncate callback.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);

    // Find the file in the FileList
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; // File not found
    }

    // If the requested size is larger, expand the file
    if (size > file->stat.st_size) {
        char *new_data = realloc(file->data, size);
        if (!new_data) {
            log_debug("ERROR: Memory allocation failed during truncate.");
            return -ENOMEM; // Out of memory
        }
        file->data = new_data;

        // Zero-fill the newly allocated space
        memset(file->data + file->stat.st_size, 0, size - file->stat.st_size);

        snprintf(log_message, sizeof(log_message), "INFO: File expanded to %ld bytes: %s", size, file_name);
        log_debug(log_message);
    } 
    // If the requested size is smaller, truncate the file
    else if (size < file->stat.st_size) {
        char *new_data = realloc(file->data, size);
        if (!new_data && size > 0) {
            log_debug("ERROR: Memory allocation failed during truncate.");
            return -ENOMEM; // Out of memory
        }
        file->data = new_data;

        snprintf(log_message, sizeof(log_message), "INFO: File truncated to %ld bytes: %s", size, file_name);
        log_debug(log_message);
    }

    // Update file metadata
    file->stat.st_size = size;
    log_debug("Outside the truncate callback.");

    return 0; // Success
}

const char* find_imei(const char *device_name, const char *json_path) {
    char log_message[512];

    // Open the JSON file
    FILE *file = fopen(json_path, "r");
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file: %s", json_path);
        log_debug(log_message);
        return NULL;  // Return NULL if file can't be opened
    }

    // Read the file contents into a string
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(size + 1);
    if (!data) {
        fclose(file);
        snprintf(log_message, sizeof(log_message), "ERROR: Memory allocation failed.");
        log_debug(log_message);
        return NULL;
    }

    fread(data, 1, size, file);
    data[size] = '\0';  // Null-terminate the string
    fclose(file);

    // Parse the JSON data
    struct json_object *root = json_tokener_parse(data);
    free(data);

    if (!root) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to parse JSON.");
        log_debug(log_message);
        return NULL;
    }

    // Retrieve the "devices" array
    struct json_object *devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Devices array not found in JSON.");
        log_debug(log_message);
        json_object_put(root);
        return NULL;
    }

    // Iterate through devices to find the device by name and return its IMEI
    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object *device = json_object_array_get_idx(devices_array, i);
        struct json_object *name_obj = NULL;

        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), device_name) == 0) {
            // Device found, now extract its IMEI
            struct json_object *imei_obj = NULL;
            if (json_object_object_get_ex(device, "IMEI", &imei_obj)) {
                const char *imei = json_object_get_string(imei_obj);
                json_object_put(root);  // Free the root object
                return imei;  // Return the IMEI
            } else {
                snprintf(log_message, sizeof(log_message), "IMEI not found for device: %s", device_name);
                log_debug(log_message);
                json_object_put(root);
                return NULL;  // Return NULL if IMEI is not found
            }
        }
    }

    // Device not found
    snprintf(log_message, sizeof(log_message), "Device '%s' not found in JSON.", device_name);
    log_debug(log_message);
    json_object_put(root);
    return NULL;  // Return NULL if device is not found
}

struct json_object* find_device(const char* device_name, const char* json_path) {
    char log_message[512];
    if (device_name == NULL || json_path == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid arguments passed to find_device.");
        log_debug(log_message);
        return NULL;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Entering find_device function.");
    log_debug(log_message);

    // Load the JSON file
    FILE* file = fopen(json_path, "r");
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file: %s", json_path);
        log_debug(log_message);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = malloc(size + 1);
    if (!data) {
        fclose(file);
        snprintf(log_message, sizeof(log_message), "ERROR: Memory allocation failed.");
        log_debug(log_message);
        return NULL;
    }

    fread(data, 1, size, file);
    data[size] = '\0';
    fclose(file);

    struct json_object* root = json_tokener_parse(data);
    free(data);

    if (!root) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to parse JSON file.");
        log_debug(log_message);
        return NULL;
    }

    // Retrieve the "devices" array
    struct json_object* devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Devices array not found in JSON.");
        log_debug(log_message);
        json_object_put(root);
        return NULL;
    }

    // Search for the device by name
    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object* device = json_object_array_get_idx(devices_array, i);
        struct json_object* name_obj = NULL;

        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), device_name) == 0) {
            json_object_get(device);  // Increment ref count to return it safely
            json_object_put(root);   // Free the root object
            return device;
        }

        // Check if this is a folder with children
        struct json_object* type_obj = NULL;
        struct json_object* children_obj = NULL;
        if (json_object_object_get_ex(device, "Type", &type_obj) &&
            strcmp(json_object_get_string(type_obj), "Folder") == 0 &&
            json_object_object_get_ex(device, "Children", &children_obj)) {
            for (int j = 0; j < json_object_array_length(children_obj); j++) {
                struct json_object* child = json_object_array_get_idx(children_obj, j);
                struct json_object* child_name = NULL;

                if (json_object_object_get_ex(child, "Name", &child_name) &&
                    strcmp(json_object_get_string(child_name), device_name) == 0) {
                    json_object_get(child);  // Increment ref count to return it safely
                    json_object_put(root);  // Free the root object
                    return child;
                }
            }
        }
    }

    snprintf(log_message, sizeof(log_message), "INFO: Device '%s' not found.", device_name);
    log_debug(log_message);
    json_object_put(root);
    return NULL;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .create = create_callback,
  .read = read_callback,
  .write = write_callback,
  .readdir = readdir_callback,
  .init = init_callback,
  .truncate = truncate_callback,
  .mkdir = mkdir_callback,
  .utimens = utimens_callback,
  .rmdir = rmdir_callback,
  .unlink = unlink_callback
};

int main(int argc, char *argv[])
{

  init_file_list(&file_list,10);
  init_dir_list(&dir_list,10);
  int result = fuse_main(argc, argv, &fuse_example_operations, NULL);

  // Clean up resources before exiting
  free_file_list(&file_list);
  free_dir_list(&dir_list);
  if(device_storage != NULL) free(device_storage);
  return result;

}

