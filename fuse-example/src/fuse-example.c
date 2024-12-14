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
static const char *important_log_file_path = "/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/important_log_file.txt";
const char *json_path = "/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/json_test_example.json";


struct json_object* find_device(const char* device_name, const char* json_path) {
    char log_message[512];
    char *real_device_name = strtok(strdup(device_name), ".");
    snprintf(log_message, sizeof(log_message), "INFO: %s.", real_device_name);
    log_debug(log_message);
    if (device_name == NULL || json_path == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid arguments passed to find_device.");
        log_debug(log_message);
        return NULL;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Entering find_device function.");
    log_debug(log_message);

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

    struct json_object* devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Devices array not found in JSON.");
        log_debug(log_message);
        json_object_put(root);
        return NULL;
    }

    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object* device = json_object_array_get_idx(devices_array, i);
        struct json_object* name_obj = NULL;

        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), real_device_name) == 0) {
            json_object_get(device);  
            json_object_put(root); 
            return device;
        }
        struct json_object* type_obj = NULL;
        struct json_object* children_obj = NULL;
        if (json_object_object_get_ex(device, "Type", &type_obj) &&
            strcmp(json_object_get_string(type_obj), "Folder") == 0 &&
            json_object_object_get_ex(device, "Children", &children_obj)) {
            for (int j = 0; j < json_object_array_length(children_obj); j++) {
                struct json_object* child = json_object_array_get_idx(children_obj, j);
                struct json_object* child_name = NULL;

                if (json_object_object_get_ex(child, "Name", &child_name) &&
                    strcmp(json_object_get_string(child_name), real_device_name) == 0) {
                    json_object_get(child);  
                    json_object_put(root);  
                    return child;
                }
            }
        }
    }

    snprintf(log_message, sizeof(log_message), "INFO: Device '%s' not found.", real_device_name);
    log_debug(log_message);
    json_object_put(root);
    return NULL;
}

const char* find_imei(char *device_name, char *json_path) {
    char log_message[512];

    FILE *file = fopen(json_path, "r");
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file: %s", json_path);
        log_debug(log_message);
        return NULL; 
    }

    
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
    data[size] = '\0';  
    fclose(file);

    
    struct json_object *root = json_tokener_parse(data);
    free(data);

    if (!root) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to parse JSON.");
        log_debug(log_message);
        return NULL;
    }

    
    struct json_object *devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Devices array not found in JSON.");
        log_debug(log_message);
        json_object_put(root);
        return NULL;
    }

    
    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object *device = json_object_array_get_idx(devices_array, i);
        struct json_object *name_obj = NULL;

        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), device_name) == 0) {
            
            struct json_object *imei_obj = NULL;
            if (json_object_object_get_ex(device, "IMEI", &imei_obj)) {
                const char *imei = json_object_get_string(imei_obj);
                json_object_put(root);  
                return imei;  
            } else {
                snprintf(log_message, sizeof(log_message), "IMEI not found for device: %s", device_name);
                log_debug(log_message);
                json_object_put(root);
                return NULL;  
            }
        }
    }

    
    snprintf(log_message, sizeof(log_message), "Device '%s' not found in JSON.", device_name);
    log_debug(log_message);
    json_object_put(root);
    return NULL;  
}

extern void log_debug(const char *message) {
    
    FILE *log_file = fopen(log_file_path, "a");
    
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

extern void important_log_debug(const char *message) {
    
    FILE *log_file = fopen(important_log_file_path, "a");
    
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}


typedef struct {
    char *name;
    int serial_number;
    char *imei;
    char *model;
} ParsedInput;

typedef struct {
    struct stat stat;  
    char *name;        
    char *directory;   
    char *data;        
    size_t capacity;   
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
        free(list->dirs[i]);  
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
    if(!strcmp(name,"GYRO") || !strcmp(name,"IMEI") || !strcmp(name,"GPS")) {
        log_debug("log_message");
        new_file->data = (char*)calloc(10,sizeof(char)); 
        new_file->stat.st_mode = __S_IFREG | 0444;
        if(!strcmp(name,"GYRO")){ 
            log_debug("inside strcmp for GYRO.");
            size_t len = strlen(new_file->data);
            new_file->data[len] = '0' + rand()%10;
            new_file->data[len+1] = ' ';
            new_file->data[len + 2] = '0' + rand()%10;
            new_file->data[len+3] = ' '; 
            new_file->data[len + 4] = '0' + rand()%10; 
            new_file->data[len + 5] = '\n';
            new_file->data[len + 6] = '\0';
        } 
        if(!strcmp(name,"GPS")){
            log_debug("inside strcmp for GPS.");
            size_t len = strlen(new_file->data);
            new_file->data[len] = '0' + rand()%10;
            new_file->data[len+1] = ' ';
            new_file->data[len + 2] = '0' + rand()%10; 
            new_file->data[len + 3] = '\n';
            new_file->data[len + 4] = '\0';
        }    
        if(!strcmp(name,"IMEI")){
            log_debug("inside strcmp for IMEI.");
            char *dir = strtok(directory,".");
            const char* dev_imei = find_imei(dir+1,json_path);
            new_file->data = strdup(dev_imei);
            new_file->data[strlen(new_file->data)] = '\n';
        }
        new_file->stat.st_size = strlen(new_file->data);
        new_file->directory = strdup(directory);
    }  
    else{
        new_file->data = (char*)calloc(512,sizeof(char));
        new_file->directory = strdup(directory);
        char* model = strrchr(name,'.');
        log_debug("log_message");
        if(!strcmp(model+1,"ACTUATOR")) new_file->stat.st_mode = __S_IFREG | 0222;
        else if(!strcmp(model+1,"SENSOR")) {
            char* helper_string[10];
            generate_random_string(helper_string,8);
            new_file->data = strdup(helper_string);
            new_file->stat.st_mode = __S_IFREG | 0444;
        }
        else {
            new_file->stat.st_mode = __S_IFREG | 0644;
        }  
        new_file->stat.st_size = strlen(new_file->data);
    }

    new_file->stat.st_nlink = 1;
    new_file->stat.st_uid = getuid();
    new_file->stat.st_gid = getgid();
    new_file->stat.st_atime = time(NULL);
    new_file->stat.st_mtime = time(NULL);
    new_file->stat.st_ctime = time(NULL);

    new_file->capacity = 0;     

    list->files[list->size++] = new_file;

    
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: Added file: %s in directory: %s", name, directory);
    log_debug(log_message);
}

const char *extract_directory_name(const char *path) {
    
    const char *last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        return path;  
    }

    
    return last_slash + 1;
}

void get_parent_directory(const char *path, char *parent) {
    char log_message[512];

    
    snprintf(parent, 512, "%s", path);  

    
    char *last_slash = strrchr(parent, '/');

    
    if (last_slash == parent) {
        snprintf(parent, 512, "/");
        return;
    }

    
    if (last_slash != NULL) {
        *last_slash = '\0';  
    }

    snprintf(log_message, sizeof(log_message), "DEBUG: Parent directory is %s", parent);
    log_debug(log_message);
}

File *find_file(FileList *list, const char *name, const char *directory) {
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->files[i]->name, name) == 0 && strcmp(list->files[i]->directory, directory) == 0) {
            
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

    
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -1;  
    }

    
    snprintf(log_message, sizeof(log_message), "INFO: Calculated size for file: %s is %ld bytes.", file_name, file->stat.st_size);
    log_debug(log_message);

    return file->stat.st_size;
}

long calculate_directory_size(const char *dir_path) {
    long total_size = 0;
    log_debug("inside calc dir size.");
    
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
        int key = rand() % charset_size; 
        random_string[i] = charset[key];
    }
    random_string[length] = '\n';
    random_string[length+1] = '\0'; 
}

void modify_path(const char *path, const char *directory_name, char *new_path) {
    
    const char *last_slash = strrchr(path, '/');

    
    const char *dot_pos = strchr(directory_name, '.');

    
    char modified_directory[256];  
    if (dot_pos != NULL) {
        size_t len = dot_pos - directory_name;
        strncpy(modified_directory, directory_name, len);
        modified_directory[len] = '\0';  
    } else {
        
        strcpy(modified_directory, directory_name);
    }

    
    size_t path_length = last_slash - path + 1;  
    strncpy(new_path, path, path_length);
    new_path[path_length] = '\0';  

    
    strcat(new_path, modified_directory);
}

void add_dir(DirList *dir_list, const char *dir_path) {
    
    for (size_t i = 0; i < dir_list->size; i++) {
        if (strcmp(dir_list->dirs[i], dir_path) == 0) {
            return;  
        }
    }

    
    if (dir_list->size == dir_list->capacity) {
        dir_list->capacity *= 2;
        dir_list->dirs = realloc(dir_list->dirs, dir_list->capacity * sizeof(char *));
        dir_list->stats = realloc(dir_list->stats, dir_list->capacity * sizeof(struct stat));
        if (!dir_list->dirs || !dir_list->stats) {
            perror("Failed to resize directory list");
            exit(EXIT_FAILURE);
        }
    }

    
    dir_list->dirs[dir_list->size] = strdup(dir_path);

    
    dir_list->stats[dir_list->size].st_size = 0; 
    dir_list->stats[dir_list->size].st_mode = S_IFDIR | 0755;  
    dir_list->stats[dir_list->size].st_uid = getuid();  
    dir_list->stats[dir_list->size].st_gid = getgid();  
    dir_list->stats[dir_list->size].st_atime = time(NULL);
    dir_list->stats[dir_list->size].st_mtime = time(NULL);
    dir_list->stats[dir_list->size].st_ctime = time(NULL);

    dir_list->size++;
}


void get_substring_up_to_char(const char *input, char *output, char delimiter) {

    
    const char *delimiter_pos = strrchr(input, delimiter);
    
    if (delimiter_pos != NULL) {
        
        size_t length = delimiter_pos - input;
        strncpy(output, input, length);
        output[length] = '\0';  
    } else {
        
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
    memset(stbuf, 0, sizeof(struct stat));  
    int dot_counter = 0;
    count_dots(extract_directory_name(path),&dot_counter);

    char parent_dir[1024];
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0775;
        stbuf->st_nlink = 2;
        stbuf->st_size = 0;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_atime = dir_list.stats[0].st_atime;
        stbuf->st_mtime = dir_list.stats[0].st_mtime;
        stbuf->st_ctime = dir_list.stats[0].st_ctime;
        return 0;
    }
    char* new_path = strdup(path);
    const char *dir_name = extract_directory_name(path);
    
    
    if(dot_counter == 2){
        modify_path(path,dir_name,new_path);
    }
    
    int dir_index = find_dir(&dir_list, new_path);

    if (dir_index != -1) {
        stbuf->st_mode = S_IFDIR | 0755;  
        stbuf->st_nlink = 2;
        stbuf->st_size = 0;  
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();   
        stbuf->st_atime = dir_list.stats[dir_index].st_atime;
        stbuf->st_mtime = dir_list.stats[dir_index].st_mtime;
        stbuf->st_ctime = dir_list.stats[dir_index].st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for directory: %s, its size is: %d.", new_path,stbuf->st_size);
        log_debug(log_message);
        return 0;
    }

    char* secondary_path = strdup(path);
    if(dot_counter == 2){
        modify_path_to_remove_serial(path,secondary_path);
    }
    get_parent_directory(secondary_path, parent_dir);

    const char *file_name = extract_directory_name(secondary_path);

    File *file = find_file(&file_list, file_name, parent_dir);

    if (file) { 
        stbuf->st_mode = file->stat.st_mode;
        stbuf->st_size = file->stat.st_size;
        stbuf->st_nlink = file->stat.st_nlink;
        stbuf->st_uid = file->stat.st_uid;
        stbuf->st_gid = file->stat.st_gid;
        stbuf->st_atime = file->stat.st_atime;
        stbuf->st_mtime = file->stat.st_mtime;
        stbuf->st_ctime = file->stat.st_ctime;

        snprintf(log_message, sizeof(log_message), "DEBUG: getattr for file: %s in directory: %s. Its size is: %d.", file_name, parent_dir,stbuf->st_size);
        log_debug(log_message);
        return 0;
    }

    
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

    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    snprintf(log_message, sizeof(log_message), "DEBUG: Reading after main fillers.");
    log_debug(log_message);

    
    for (size_t i = 0; i < dir_list.size; i++) {       
        
        char parent_dir[1024];
        get_parent_directory(dir_list.dirs[i], parent_dir);
        snprintf(log_message, sizeof(log_message), "DEBUG: Parent_dir: %s and path: %s", parent_dir,path);
        log_debug(log_message);

        if (strcmp(parent_dir, path) == 0) {
            const char *base_name = extract_directory_name(dir_list.dirs[i]);
            snprintf(log_message,sizeof(log_message),"Extracted dir name is::: %s.",base_name);
            log_debug(log_message);
            filler(buf, base_name, NULL, 0);
            snprintf(log_message, sizeof(log_message), "DEBUG: Listed directory: %s", base_name);
            log_debug(log_message);
        }
    }
    
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
    
    FILE *important_log_file = fopen(important_log_file_path, "w");
    if(important_log_file){
        fclose(important_log_file);
    }
    FILE *log_file = fopen(log_file_path, "w");
    if (log_file) {
        fclose(log_file);  
    }
    FILE *json_file = fopen(json_path,"w");
    if (json_file) {
        fclose(json_file);  
    }
    
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
        return 0;  
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

    
    int dir_index = find_dir(&dir_list, secondary_path);
    if (dir_index != -1) {
        
        dir_list.stats[dir_index].st_atime = tv ? tv[0].tv_sec : time(NULL);
        dir_list.stats[dir_index].st_mtime = tv ? tv[1].tv_sec : time(NULL);

        snprintf(log_message, sizeof(log_message), "DEBUG: Updated timestamps for directory: %s", secondary_path);
        log_debug(log_message);
        return 0;
    }

    
    File *file = find_file(&file_list, file_name, parent_dir);
    if (file) {
        
        file->stat.st_atime = tv ? tv[0].tv_sec : time(NULL);
        file->stat.st_mtime = tv ? tv[1].tv_sec : time(NULL);

        int parent_dir_index = find_dir(&dir_list, parent_dir);
        if (parent_dir_index != -1) {
            
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

    
    snprintf(log_message, sizeof(log_message), "DEBUG: Timestamps update failed: %s not found", secondary_path);
    log_debug(log_message);
    return -ENOENT;
}

int check_restrictions(const char *input, const char *parent_directory, ParsedInput* parsed_input) {
    char log_message[512];

    regex_t regex;
    const char *pattern = "^[a-zA-Z0-9_]+\\.[a-zA-Z0-9_-]+\\.[0-9]+$";

    
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        log_debug("ERROR: Failed to compile regex.");
        return -EINVAL;
    }

    
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

    
    if (!is_valid_model(string2, FILE_TYPE)) {
        snprintf(log_message,sizeof(log_message),"ERROR: Invalid model specified, %s.",string2);
        log_debug(log_message);
        free(copy);
        return 0;
    }

    
    if (strcmp(parent_directory, "/") == 0) {
        snprintf(log_message,sizeof(log_message),"ERROR: Files cannot be created in the root directory.");
        log_debug(log_message);
        free(copy);
        return 0;
    }

    
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
    (void) fi;  

    char log_message[512];
    time_t registration_date = time(NULL);
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    if(!strcmp(file_name,"GYRO") || !strcmp(file_name,"IMEI") || !strcmp(file_name,"GPS")){
        if (find_file(&file_list, file_name, parent_dir) != NULL) {
            return -EEXIST;  
        }
        snprintf(log_message, sizeof(log_message), "DEBUG: Creating %s file.", file_name);
        log_debug(log_message);
        add_file(&file_list,file_name,parent_dir);
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

    
    if (find_file(&file_list, real_file_name, parent_dir) != NULL) {
        return -EEXIST;  
    }

    
    if (find_dir(&dir_list, parent_dir) == -1) {
        return -ENOENT;  
    }
    DeviceEntry *device = create_and_add_device_entry(
        parsed_input.name, parsed_input.model, parsed_input.serial_number, registration_date, parsed_input.imei, FILE_TYPE);
    
    if(device == NULL){
        snprintf(log_message, sizeof(log_message), "DEBUG: Device is null.");
        log_debug(log_message);
    }
    add_file(&file_list, real_file_name, parent_dir);
    add_device_to_json(device, json_path, extract_directory_name(parent_dir));

    
    snprintf(log_message, sizeof(log_message), "DEBUG: File created successfully: %s in directory: %s", real_file_name, parent_dir);
    log_debug(log_message);

    struct timespec ts[2];
    clock_gettime(CLOCK_REALTIME, &ts[0]);  
    ts[1] = ts[0];  
    

    return 0;  
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    log_debug("Inside read callback function.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    char* file_name = extract_directory_name(path);
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; 
    }
    char* model = strrchr(file_name,'.');
    if(model!= NULL && !strcmp(model+1,"ACTUATOR")) return -EPERM;
    if (offset + size > strlen(file->data)) {
        memcpy(buf, file->data + offset, strlen(file->data) - offset);
        return strlen(file->data) - offset;
    }
    memcpy(buf, file->data + offset, size);
    
    if(!strcmp(file->read_type,"data")){
        snprintf(log_message, sizeof(log_message),"%s",file->data);
        important_log_debug(log_message);
    }
    if(!strcmp(file->read_type,"info")){
        snprintf(log_message,sizeof(log_message),"[%s] : info",file_name);
        important_log_debug(log_message);
    }

    return size;
}

static int validate_and_parse_mkdir_input(const char *dir_name, ParsedInput *parsed) {
    regex_t regex;
    const char *pattern = "^[a-zA-Z0-9_]+\\.[a-zA-Z0-9_-]+\\.[0-9]+$";

    
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        log_debug("ERROR: Failed to compile regex.");
        return -EINVAL;
    }

    
    if (regexec(&regex, dir_name, 0, NULL, 0) != 0) {
        log_debug("ERROR: Directory name format is invalid. Expected format: name.serial_number.imei");
        regfree(&regex);
        return -EINVAL;
    }
    regfree(&regex);

    
    char *name = strtok(strdup(dir_name), ".");
    char *serial_number_str = strtok(NULL, ".");
    char *imei_str = strtok(NULL, ".");

    
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

    parsed->name = name;
    parsed->serial_number = atoi(serial_number_str);
    parsed->imei = imei_str;
    parsed->model = NULL;

    return 0;  
}

static int mkdir_callback(const char *path, mode_t permission_bits) {
    char log_message[512];
    snprintf(log_message, sizeof(log_message), "DEBUG: mkdir_callback called with path = %s, permissions = %o", path, permission_bits);
    log_debug(log_message);

    
    if (strchr(path + 1, '/') != NULL) {  
        log_debug("ERROR: Directories can only be created in the root.");
        return -EPERM;  
    }
    
    const char *dir_name = extract_directory_name(path);
    char new_path[512];
    modify_path(path, dir_name, new_path);

    ParsedInput parsed;
    int validation_result = validate_and_parse_mkdir_input(dir_name, &parsed);
    if (validation_result != 0) {
        return validation_result;  
    }

    if (find_dir(&dir_list, new_path) != -1) {
        log_debug("ERROR: Directory already exists.");
        free(parsed.name);
        return -EEXIST;  
    }
    add_dir(&dir_list, new_path);
    time_t registration_date = time(NULL);
    DeviceEntry *device = create_and_add_device_entry(
        parsed.name, "TTConnectWave", parsed.serial_number, registration_date, parsed.imei, FOLDER_TYPE);

    if (!device) {
        log_debug("ERROR: Failed to create device entry.");
        free(parsed.name);
        return -ENOMEM;  
    }
    const char *parent_name = "/";  
    add_device_to_json(device, json_path, parent_name);
    snprintf(log_message, sizeof(log_message), "INFO: Directory %s created successfully and device added to JSON.", new_path);
    log_debug(log_message);
    free(parsed.name);
    char* helper_string = strdup(new_path);
    create_callback(strcat(helper_string,"/IMEI"),__S_IFREG,NULL);
    snprintf(log_message, sizeof(log_message), "INFO: Path is: %s.", helper_string);
    log_debug(log_message);
    helper_string = strdup(new_path);
    create_callback(strcat(helper_string,"/GPS"),__S_IFREG,NULL);
    snprintf(log_message, sizeof(log_message), "INFO: Path is: %s.", helper_string);
    log_debug(log_message);
    helper_string = strdup(new_path);
    create_callback(strcat(helper_string,"/GYRO"),__S_IFREG,NULL);
    snprintf(log_message, sizeof(log_message), "INFO: Path is: %s.", helper_string);
    log_debug(log_message);
    return 0;
}

void remove_dir(DirList *list, size_t index) {
    if (index >= list->size) {
        return;  
    }
    free(list->dirs[index]);

    
    for (size_t i = index; i < list->size - 1; i++) {
        list->dirs[i] = list->dirs[i + 1];
        list->stats[i] = list->stats[i + 1];
    }

    
    list->size--;
}
void remove_file(FileList *file_list, const char *path) {
    char log_message[512];

    char parent_dir[512];
    get_parent_directory(path, parent_dir);
    
    const char *file_name = strrchr(path, '/');
    if (!file_name) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid path format: %s", path);
        log_debug(log_message);
        return;
    }
    file_name++;  

    
    File *file = find_file(file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return;
    }

    
    snprintf(log_message, sizeof(log_message), "INFO: Removing file: %s from directory: %s", file_name, parent_dir);
    log_debug(log_message);

    
    for (size_t i = 0; i < file_list->size; i++) {
        if (file_list->files[i] == file) {
            
            free(file->name);
            free(file->directory);
            free(file->data);
            free(file);

            
            for (size_t j = i; j < file_list->size - 1; j++) {
                file_list->files[j] = file_list->files[j + 1];
            }

            
            file_list->size--;

            
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
    
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    char *file_name = extract_directory_name(path);

    if (find_file(&file_list, file_name, parent_dir) == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT;  
    }

    remove_file(&file_list, path);

    char* real_file_name = (char*)calloc(20,sizeof(char));
    get_substring_up_to_char(file_name,real_file_name,'.');    

    remove_device_from_json(real_file_name,json_path);

    snprintf(log_message, sizeof(log_message), "INFO: File successfully unlinked: %s", path);
    log_debug(log_message);
    free(real_file_name);
    return 0;  
}


static int rmdir_callback(const char *path) {
    
    if(!strcmp(extract_directory_name(path),"GYRO") || !strcmp(extract_directory_name(path),"GPS") || !strcmp(extract_directory_name(path),"IMEI")) return 0;
    int dir_index = find_dir(&dir_list, path);
    if (dir_index == -1) {
        return -ENOENT;  
    }
    char* new_path = strdup(path);
    unlink_callback(strcat(new_path,"/GPS"));
    new_path = strdup(path);
    unlink_callback(strcat(new_path,"/GYRO"));
    new_path = strdup(path);
    unlink_callback(strcat(path,"/IMEI"));
    remove_dir(&dir_list, dir_index);

    remove_device_from_json(extract_directory_name(path),json_path);
    return 0;  
}


static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    log_debug("Inside write callback function.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);
    char* model = strrchr(file_name,'.');
    if(model != NULL && !strcmp(model+1,"SENSOR")) return -EPERM;
    size_t required_capacity = offset + size;
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; 
    }
    char* dev_model = strrchr(file_name,'.') + 1;
    if(!strcmp(dev_model,"ACTUATOR")){
        snprintf(log_message,sizeof(log_message),"[%s] : %s",file_name,buf);
        important_log_debug(log_message);
        file->data = strdup(buf);
        file->stat.st_size = strlen(buf);
        file->stat.st_mtime = time(NULL); 
        return size;
    }
    else if(!strcmp(buf,"data\n")){
        strcpy(file->read_type,"data");
        char helper_string[128];
        generate_random_string(helper_string,8);
        file->data = strdup(helper_string);
        snprintf(log_message,sizeof(log_message),"[%s] : data",file_name);
        important_log_debug(log_message);
        file->stat.st_size = strlen(file->data);
        file->stat.st_mtime = time(NULL); 
        return size;
    } 
    else if(!strcmp(buf,"info\n")){
        free(file->data);
        file->data = (char*)calloc(512,sizeof(char));
        snprintf(log_message,sizeof(log_message),"[%s] : info",file_name);
        important_log_debug(log_message);
        strcpy(file->read_type,"info");
        struct json_object *device = find_device(file->name, json_path);
        struct json_object *name_obj;
        struct json_object *ser_num;
        struct json_object *reg_date;
        struct json_object *sys_id;
        struct json_object *model;
        if (json_object_object_get_ex(device, "Name", &name_obj)){
            snprintf(log_message,sizeof(log_message),"Device Name: %s\n", json_object_get_string(name_obj));
            strcat(file->data,log_message);
        }
        if (json_object_object_get_ex(device, "Model", &model)){
            snprintf(log_message,sizeof(log_message),"Device model: %s\n", json_object_get_string(model));
            strcat(file->data,log_message);
        }
        if(json_object_object_get_ex(device, "SerialNumber", &ser_num)){
            snprintf(log_message,sizeof(log_message),"Device serial num: %s\n", json_object_get_string(ser_num));
            strcat(file->data,log_message);
        }
        if(json_object_object_get_ex(device, "RegistrationDate", &reg_date)){
            snprintf(log_message,sizeof(log_message),"Device reg date: %s\n", json_object_get_string(reg_date));
            strcat(file->data,log_message);
        }
        if(json_object_object_get_ex(device, "System id", &sys_id)){
            snprintf(log_message,sizeof(log_message),"Device sys id: %s\n", json_object_get_string(sys_id));
            strcat(file->data,log_message);
        }
        file->stat.st_size = strlen(file->data);
        file->stat.st_mtime = time(NULL); 
        return size;
    }
    else{
        important_log_debug("ERROR: invalid writing.");
        return -EPERM;
    }
}

static int truncate_callback(const char *path, off_t size) {
    log_debug("Inside the truncate callback.");
    char log_message[512];
    char parent_dir[1024];
    get_parent_directory(path, parent_dir);
    const char *file_name = extract_directory_name(path);

    
    File *file = find_file(&file_list, file_name, parent_dir);
    if (!file) {
        snprintf(log_message, sizeof(log_message), "ERROR: File not found: %s in directory: %s", file_name, parent_dir);
        log_debug(log_message);
        return -ENOENT; 
    }

    
    if (size > file->stat.st_size) {
        char *new_data = realloc(file->data, size);
        if (!new_data) {
            log_debug("ERROR: Memory allocation failed during truncate.");
            return -ENOMEM; 
        }
        file->data = new_data;

        
        memset(file->data + file->stat.st_size, 0, size - file->stat.st_size);

        snprintf(log_message, sizeof(log_message), "INFO: File expanded to %ld bytes: %s", size, file_name);
        log_debug(log_message);
    } 
    
    else if (size < file->stat.st_size) {
        char *new_data = realloc(file->data, size);
        if (!new_data && size > 0) {
            log_debug("ERROR: Memory allocation failed during truncate.");
            return -ENOMEM; 
        }
        file->data = new_data;

        snprintf(log_message, sizeof(log_message), "INFO: File truncated to %ld bytes: %s", size, file_name);
        log_debug(log_message);
    }

    
    file->stat.st_size = size;
    log_debug("Outside the truncate callback.");

    return 0; 
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
  
  free_file_list(&file_list);
  free_dir_list(&dir_list);
  if(device_storage != NULL) free(device_storage);
  return result;

}

