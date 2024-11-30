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
        snprintf(log_message, sizeof(log_message), "INFO: Error when adding the device to json file");
        log_debug(log_message);
        return;
    }
    snprintf(log_message, sizeof(log_message), "INFO: In add_dev func");
    log_debug(log_message);
    // Load the existing JSON file
    FILE *file = fopen(json_path, "r");
    struct json_object *root = NULL;

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *json_data = malloc(file_size + 1);
        fread(json_data, 1, file_size, file);
        json_data[file_size] = '\0';
        fclose(file);

        root = json_tokener_parse(json_data);
        free(json_data);
        snprintf(log_message, sizeof(log_message), "INFO: Error: File is not null");
        log_debug(log_message);
    }

    // If the JSON file doesn't exist or is empty, initialize a new root object
    if (root == NULL) {
        root = json_object_new_object();
    }

    // Recursive function to find the parent folder and add the device
    

    // Create the JSON object for the new device
    struct json_object *device_json = json_object_new_object();
    json_object_object_add(device_json, "Name", json_object_new_string(device->name));
    json_object_object_add(device_json, "Model", json_object_new_string(device->model));
    json_object_object_add(device_json, "SerialNumber", json_object_new_int(device->serial_number));
    json_object_object_add(device_json, "RegistrationDate", json_object_new_int64(device->registration_date));
    json_object_object_add(device_json, "IMEI", json_object_new_string(device->imei));
    json_object_object_add(device_json, "Type",
                           json_object_new_string(device->type == FOLDER_TYPE ? "Folder" : "File"));

    // Only add "Children" if it's a folder
    if (device->type == FOLDER_TYPE) {
        json_object_object_add(device_json, "Children", json_object_new_array());
    }

    // Add the device to the correct parent folder
    add_to_parent(root, parent_name, device_json);

    // Save the updated JSON back to the file
    file = fopen(json_path, "w+");
    if (file == NULL) {
        snprintf(log_message, sizeof(log_message), "INFO: Error: Failed to open json file for writing");
        log_debug(log_message);
        return;
    }

    fprintf(file, "%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(file);

    // Free the root JSON object
    json_object_put(root);
}

