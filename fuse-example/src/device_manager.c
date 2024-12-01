#include "device_manager.h"
#include<json-c/json.h>

DeviceEntry* device_storage = NULL;
int device_count = 0; 
int device_capacity = 0;

void ensure_device_capacity() {
    if (device_storage == NULL) {
        // Initial allocation for device storage
        device_capacity = INITIAL_CAPACITY;
        device_storage = (DeviceEntry *)malloc(sizeof(DeviceEntry) * device_capacity);
        if (device_storage == NULL) {
            fprintf(stderr, "Failed to allocate memory for device storage.\n");
            exit(EXIT_FAILURE);
        }
    } else if (device_count >= device_capacity) {
        // Expand the device storage when capacity is exceeded
        device_capacity *= 2; // Double the capacity
        DeviceEntry *new_storage = (DeviceEntry *)realloc(device_storage, sizeof(DeviceEntry) * device_capacity);
        if (new_storage == NULL) {
            fprintf(stderr, "Failed to expand memory for device storage.\n");
            exit(EXIT_FAILURE);
        }
        device_storage = new_storage;
    }
}

int is_valid_model(const char *model, EntryType type) {
    // List of valid prefixes for different entry types
    const char *file_prefixes[] = {"HY-TTC_", "VISION_", "ACTUATOR", "SENSOR"};
    const char *folder_prefix = "TTConnectWave";

    // If it's a folder, the model must match the folder prefix
    if (type == FOLDER_TYPE) {
        if(strncmp(model, folder_prefix, strlen(folder_prefix)) == 0) return 1;
        else return 0;
    }

    // If it's a file, check if the model starts with one of the valid file prefixes
    for (size_t i = 0; i < sizeof(file_prefixes) / sizeof(file_prefixes[0]); i++) {
        if (strncmp(model, file_prefixes[i], strlen(file_prefixes[i])) == 0) {
            return 1;
        }
    }

    // If no valid prefix matches, the model is invalid
    return 0;
}


DeviceEntry *create_and_add_device_entry(const char *name, const char *model, 
                                         int serial_number, time_t registration_date, 
                                         char* imei, EntryType type) {
    // Ensure there is enough space in the device storage
    ensure_device_capacity();

    // Validate the model
    if (!is_valid_model(model, type)) {
        fprintf(stderr, "Invalid model type: %s\n", model);
        return NULL;
    }

    // Create a new device entry and populate the fields
    DeviceEntry *entry = &device_storage[device_count];
    strncpy(entry->name, name, MAX_NAME_LENGTH - 1);
    entry->name[MAX_NAME_LENGTH - 1] = '\0'; // Null-terminate in case of overflow

    if (type == FOLDER_TYPE) {
        strncpy(entry->model, "TTConnectWave", MAX_MODEL_LENGTH - 1);
    } else {
        strncpy(entry->model, model, MAX_MODEL_LENGTH - 1);
    }
    entry->model[MAX_MODEL_LENGTH - 1] = '\0'; // Null-terminate in case of overflow

    entry->serial_number = serial_number;
    entry->registration_date = registration_date;
    snprintf(entry->imei,sizeof(imei),"%7s", imei); // Format IMEI as a 7-character string

    // Generate a random system ID
    entry->system_id = rand();

    entry->type = type;

    // Increment the device count to reflect the new entry
    device_count++;

    // Return the pointer to the newly created entry
    return entry;
}

void add_to_parent(struct json_object *current, const char *parent_name, struct json_object *device_json) {
    struct json_object *name_field = NULL;
    // Check if the current object matches the parent folder
    if (json_object_object_get_ex(current, "Name", &name_field) &&
        strcmp(json_object_get_string(name_field), parent_name) == 0) {

        // Ensure the current object has a "Children" array
        struct json_object *children_array = NULL;
        if (!json_object_object_get_ex(current, "Children", &children_array)) {
            children_array = json_object_new_array();
            json_object_object_add(current, "Children", children_array);
        }

        // Add the new device to the "Children" array
        json_object_array_add(children_array, device_json);
        return;
    }
    // Recursively search in the "Children" array
    struct json_object *children_array = NULL;
    if (json_object_object_get_ex(current, "Children", &children_array)) {
        int child_count = json_object_array_length(children_array);
        for (int i = 0; i < child_count; i++) {
            struct json_object *child = json_object_array_get_idx(children_array, i);
            add_to_parent(child, parent_name, device_json);
        }
    }
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

    // Load the existing JSON file or create a new one with "Root"
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
        // Initialize a new JSON structure with "devices"
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
            json_object_put(root);  // Clean up root if devices_array creation fails
            return;
        }

        json_object_object_add(root, "devices", devices_array);

        snprintf(log_message, sizeof(log_message), "INFO: Initialized new JSON structure with devices array.");
        log_debug(log_message);
    }

    // Retrieve the "devices" array
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


    // Create the JSON object for the device
    struct json_object *device_json = json_object_new_object();
    json_object_object_add(device_json, "Name", json_object_new_string(device->name));
    json_object_object_add(device_json, "Model", json_object_new_string(device->model));
    json_object_object_add(device_json, "SerialNumber", json_object_new_int(device->serial_number));
    json_object_object_add(device_json, "RegistrationDate", json_object_new_int64(device->registration_date));
    json_object_object_add(device_json, "IMEI", json_object_new_string(device->imei));

    if (device->type == FOLDER_TYPE) {
        // Add a folder with an empty "Children" array
        json_object_object_add(device_json, "Type", json_object_new_string("Folder"));
        json_object_object_add(device_json, "Children", json_object_new_array());
        json_object_array_add(devices_array, device_json);

        snprintf(log_message, sizeof(log_message), "INFO: Folder added to devices: %s", device->name);
        log_debug(log_message);

    } else if (device->type == FILE_TYPE) {
        // Add a file to the "Children" array of the specified parent folder
        snprintf(log_message, sizeof(log_message), "INFO: Adding file to parent folder: %s", parent_name);
        log_debug(log_message);

        for (int i = 0; i < json_object_array_length(devices_array); i++) {
            struct json_object *folder = json_object_array_get_idx(devices_array, i);
            struct json_object *folder_name = NULL;

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

    // Save the updated JSON back to the file
    file = fopen(json_path, "w");
    if (file) {
        fprintf(file, "%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
        fclose(file);
        snprintf(log_message, sizeof(log_message), "INFO: JSON data written successfully to file.");
        log_debug(log_message);
    } else {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file for writing.");
        log_debug(log_message);
    }

    json_object_put(root);
}

// Helper function to recursively remove a folder and its children
int remove_folder_and_children(struct json_object *devices_array, const char *folder_name) {
    for (int i = 0; i < json_object_array_length(devices_array); i++) {
        struct json_object *device = json_object_array_get_idx(devices_array, i);
        struct json_object *name_obj = NULL;

        // Check if this is the folder we want to delete
        if (json_object_object_get_ex(device, "Name", &name_obj) &&
            strcmp(json_object_get_string(name_obj), folder_name) == 0) {

            // Remove the folder from the array
            json_object_array_del_idx(devices_array, i, 1);
            return 1; // Folder deleted successfully
        }

        // Check if this is a folder and recursively search its children
        struct json_object *type_obj = NULL;
        struct json_object *children_obj = NULL;
        if (json_object_object_get_ex(device, "Type", &type_obj) &&
            strcmp(json_object_get_string(type_obj), "Folder") == 0 &&
            json_object_object_get_ex(device, "Children", &children_obj)) {

            // Recursively attempt to remove the folder from children
            if (remove_folder_and_children(children_obj, folder_name)) {
                return 1; // Folder deleted successfully
            }
        }
    }
    return 0; // Folder not found
}


void remove_device_from_json(const char *folder_name, const char *json_path) {
    char log_message[512];

    if (folder_name == NULL || json_path == NULL) {
        snprintf(log_message, sizeof(log_message), "ERROR: Invalid arguments passed to remove_device_from_json.");
        log_debug(log_message);
        return;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Entering remove_device_from_json function.");
    log_debug(log_message);

    struct json_object *root = NULL;

    // Load the JSON file
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

    // Retrieve the "devices" array
    struct json_object *devices_array = NULL;
    if (!json_object_object_get_ex(root, "devices", &devices_array)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to retrieve devices array from JSON.");
        log_debug(log_message);
        json_object_put(root);
        return;
    }

    // Attempt to remove the folder and its children
    if (!remove_folder_and_children(devices_array, folder_name)) {
        snprintf(log_message, sizeof(log_message), "ERROR: Folder '%s' not found in devices array.", folder_name);
        log_debug(log_message);
        json_object_put(root);
        return;
    }

    snprintf(log_message, sizeof(log_message), "INFO: Folder '%s' removed successfully.", folder_name);
    log_debug(log_message);

    // Save the updated JSON back to the file
    file = fopen(json_path, "w");
    if (file) {
        fprintf(file, "%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
        fclose(file);
        snprintf(log_message, sizeof(log_message), "INFO: JSON data written successfully to file.");
        log_debug(log_message);
    } else {
        snprintf(log_message, sizeof(log_message), "ERROR: Failed to open JSON file for writing.");
        log_debug(log_message);
    }

    json_object_put(root);
}
