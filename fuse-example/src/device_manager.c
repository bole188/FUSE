#include "device_manager.h"
#include<json-c/json.h>

DeviceEntry* device_storage = NULL;
int device_count = 0; 
int device_capacity = 0;

void ensure_device_capacity() {
    if (device_storage == NULL) {
        device_capacity = INITIAL_CAPACITY;
        device_storage = (DeviceEntry *)malloc(sizeof(DeviceEntry) * device_capacity);
        if (device_storage == NULL) {
            exit(EXIT_FAILURE);
        }
    } else if (device_count >= device_capacity) {
        device_capacity *= 2; 
        DeviceEntry *new_storage = (DeviceEntry *)realloc(device_storage, sizeof(DeviceEntry) * device_capacity);
        if (new_storage == NULL) {
            exit(EXIT_FAILURE);
        }
        device_storage = new_storage;
    }
}

int is_valid_model(const char *model, EntryType type) {
    const char *file_prefixes[] = {"HY-TTC_", "VISION_", "ACTUATOR", "SENSOR"};
    const char *folder_prefix = "TTConnectWave";

    if (type == FOLDER_TYPE) {
        if(strncmp(model, folder_prefix, strlen(folder_prefix)) == 0) return 1;
        else return 0;
    }

    for (size_t i = 0; i < sizeof(file_prefixes) / sizeof(file_prefixes[0]); i++) {
        if (strncmp(model, file_prefixes[i], strlen(file_prefixes[i])) == 0) {
            return 1;
        }
    }

    return 0;
}

DeviceEntry *create_and_add_device_entry(const char *name, const char *model, 
                                         int serial_number, time_t registration_date, 
                                         char* imei, EntryType type) {
    ensure_device_capacity();

    if (!is_valid_model(model, type)) {
        return NULL;
    }

    DeviceEntry *entry = &device_storage[device_count];
    strncpy(entry->name, name, MAX_NAME_LENGTH - 1);
    entry->name[MAX_NAME_LENGTH - 1] = '\0';

    if (type == FOLDER_TYPE) {
        strncpy(entry->model, "TTConnectWave", MAX_MODEL_LENGTH - 1);
    } else {
        strncpy(entry->model, model, MAX_MODEL_LENGTH - 1);
    }
    entry->model[MAX_MODEL_LENGTH - 1] = '\0'; 

    entry->serial_number = serial_number;
    entry->registration_date = registration_date;
    snprintf(entry->imei,sizeof(imei),"%7s", imei);
    for(int i =0;i<7;i++){
        entry->system_id[i] = '0' + rand()%10;
    }
    entry->system_id[7] = '\0';
    
    entry->type = type;

    device_count++;

    return entry;
}


void add_device_to_json(DeviceEntry *device, const char *json_path, const char *parent_name) {
    char log_message[512];

    if (device == NULL || json_path == NULL || parent_name == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid arguments passed to add_device_to_json.");
        log_debug(log_message);
        return;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Entering add_device_to_json function.");
    log_debug(log_message);

    struct json_object *root = NULL;
    FILE *file = fopen(json_path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *data = malloc(size + 1);
        if (data) {
            fread(data, 1, size, file);
            data[size] = '\0';
            root = json_tokener_parse(data);
            free(data);
        }
        fclose(file);
    }

    if (!root) {
        root = json_object_new_object();
        if (!root) {
            snprintf(log_message, sizeof(log_message), "ERROR: Failed to initialize root JSON object.");
            log_debug(log_message);
            return;
        }

        struct json_object *devices_array = json_object_new_array();
        if (!devices_array) {
            snprintf(log_message, sizeof(log_message), "ERROR: Failed to initialize devices array.");
            log_debug(log_message);
            json_object_put(root); 
            return;
        }

        json_object_object_add(root, "devices", devices_array);

        snprintf(log_message, sizeof(log_message), "INFO: Initialized new JSON structure with devices array.");
        log_debug(log_message);
    }

    struct json_object *devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        devices_array = json_object_new_array();
        if(!devices_array){
            snprintf(log_message, sizeof(log_message), "ERROR: Failed to retrieve devices array from JSON.");
            log_debug(log_message);
            json_object_put(root);
            return;
        }
        json_object_object_add(root, "devices", devices_array);
    }

    struct json_object *device_json = json_object_new_object();
    json_object_object_add(device_json, "Name", json_object_new_string(device->name));
    json_object_object_add(device_json, "Model", json_object_new_string(device->model));
    json_object_object_add(device_json, "SerialNumber", json_object_new_int(device->serial_number));
    json_object_object_add(device_json, "RegistrationDate", json_object_new_int64(device->registration_date));
    json_object_object_add(device_json, "System id", json_object_new_string(device->system_id));
    if (device->type == FOLDER_TYPE) {
        json_object_object_add(device_json, "IMEI", json_object_new_string(device->imei));
        json_object_object_add(device_json, "Type", json_object_new_string("Folder"));
        json_object_object_add(device_json, "Children", json_object_new_array());
        json_object_array_add(devices_array, device_json);

        snprintf(log_message, sizeof(log_message), "INFO: Folder added to devices: %s", device->name);
        log_debug(log_message);

    } else if (device->type == FILE_TYPE) {
        snprintf(log_message, sizeof(log_message), "INFO: Adding file to parent folder: %s", parent_name);
        log_debug(log_message);

        for (int i = 0; i < json_object_array_length(devices_array); i++) {
            struct json_object *folder = json_object_array_get_idx(devices_array, i);
            struct json_object *folder_name = NULL;
            json_object_object_get_ex(folder, "Name", &folder_name);
            snprintf(log_message, sizeof(log_message), "INFO: Folder name: %s, and parent name: %s.", json_object_get_string(folder_name),parent_name);
            log_debug(log_message);
            if (json_object_object_get_ex(folder, "Name", &folder_name) &&
                strcmp(json_object_get_string(folder_name), parent_name) == 0) {
                struct json_object *children = NULL;

                if (!json_object_object_get_ex(folder, "Children", &children)) {
                    children = json_object_new_array();
                    json_object_object_add(folder, "Children", children);
                }

                json_object_object_add(device_json, "Type", json_object_new_string("File"));
                json_object_array_add(children, device_json);

                snprintf(log_message, sizeof(log_message), "INFO: File added to folder: %s", parent_name);
                log_debug(log_message);
                break;
            }
        }
    }
    file = fopen(json_path, "w");
    if (file) {
        fclose(file);
        snprintf(log_message, sizeof(log_message), "INFO: JSON data written successfully to file.");
        log_debug(log_message);
    } else {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file for writing.");
        log_debug(log_message);
    }

    json_object_put(root);
}

int remove_folder_and_children(struct json_object *devices_array, const char *folder_name) {
    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object *device = json_object_array_get_idx(devices_array, i);
        struct json_object *name_obj = NULL;

        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), folder_name) == 0) {

            json_object_array_del_idx(devices_array, i, 1);
            return 1; 
        }

        struct json_object *type_obj = NULL;
        struct json_object *children_obj = NULL;
        if (json_object_object_get_ex(device, "Type", &type_obj) &&
            strcmp(json_object_get_string(type_obj), "Folder") == 0 &&
            json_object_object_get_ex(device, "Children", &children_obj)) {

            if (remove_folder_and_children(children_obj, folder_name)) {
                return 1;
            }
        }
    }
    return 0; 
}

void remove_device_from_json(const char *device_name, const char *json_path) {
    char log_message[512];

    if (device_name == NULL || json_path == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid arguments passed to remove_device_from_json.");
        log_debug(log_message);
        return;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Entering remove_device_from_json function.");
    log_debug(log_message);

    struct json_object *root = NULL;

    FILE *file = fopen(json_path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *data = malloc(size + 1);
        if (data) {
            fread(data, 1, size, file);
            data[size] = '\0';
            root = json_tokener_parse(data);
            free(data);
        }
        fclose(file);
    }

    if (!root) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to load JSON file.");
        log_debug(log_message);
        return;
    }

    struct json_object *devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to retrieve devices array from JSON.");
        log_debug(log_message);
        json_object_put(root);
        return;
    }

        for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object *device = json_object_array_get_idx(devices_array, i);
        struct json_object *children = NULL;
        struct json_object *type = NULL;
        
        if (json_object_object_get_ex(device, "Type", &type) &&
            strcmp(json_object_get_string(type), "Folder") == 0 &&
            json_object_object_get_ex(device, "Children", &children)) {

            snprintf(log_message, sizeof(log_message), "Inside the if condition.");
            log_debug(log_message);

            for (int j = 0; j < json_object_array_length(children); j++) {
                struct json_object *child = json_object_array_get_idx(children, j);
                struct json_object *child_name = NULL;

                json_object_object_get_ex(child, "Name", &child_name);
                snprintf(log_message, sizeof(log_message), "%d: Child name: %s, device name: %s.",i,json_object_get_string(child_name),device_name);
                log_debug(log_message);


                if (json_object_object_get_ex(child, "Name", &child_name) &&
                    strcmp(json_object_get_string(child_name), device_name) == 0) {
                    json_object_array_del_idx(children, j, 1);

                    snprintf(log_message, sizeof(log_message), "INFO: File '%s' removed successfully.", device_name);
                    log_debug(log_message);

                    file = fopen(json_path, "w");
                    if (file) {
                        fclose(file);
                        snprintf(log_message, sizeof(log_message), "INFO: JSON data written successfully to file.");
                        log_debug(log_message);
                    } else {
                        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file for writing.");
                        log_debug(log_message);
                    }

                    json_object_put(root);
                    return;
                }
            }
        }
    }
    if (!remove_folder_and_children(devices_array, device_name)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Device '%s' not found in devices array.", device_name);
        log_debug(log_message);
        json_object_put(root);
        return;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Folder '%s' removed successfully.", device_name);
    log_debug(log_message);

    file = fopen(json_path, "w");
    if (file) {
        fclose(file);
        snprintf(log_message, sizeof(log_message), "INFO: JSON data written successfully to file.");
        log_debug(log_message);
    } else {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file for writing.");
        log_debug(log_message);
    }

    json_object_put(root);
}
