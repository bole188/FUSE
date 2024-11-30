#include "device_manager.h"

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
                                         int imei, EntryType type) {
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
    snprintf(entry->imei, sizeof(entry->imei), "%07d", imei); // Format IMEI as a 7-character string

    // Generate a random system ID
    entry->system_id = rand();

    entry->type = type;

    // Increment the device count to reflect the new entry
    device_count++;

    // Return the pointer to the newly created entry
    return entry;
}



