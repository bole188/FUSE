#include "device_manager.h"


DeviceEntry *create_and_add_device_entry(const char *name, const char *model, 
                                 int serial_number, time_t registration_date, 
                                 int imei, EntryType type) {
    // Ensure there is enough space in the device storage
    ensure_device_capacity();

    // Create a new device entry and populate the fields
    DeviceEntry *entry = &device_storage[device_count];
    strncpy(entry->name, name, MAX_NAME_LENGTH);
    strncpy(entry->model, model, MAX_MODEL_LENGTH);
    entry->serial_number = serial_number;
    entry->registration_date = registration_date;
    entry->imei = imei;
    entry->type = type;

    // Increment the device count to reflect the new entry
    device_count++;

    // Return the pointer to the newly created entry
    return entry;
}

void ensure_device_capacity() {
    if (device_count >= MAX_DEVICES) {
        // Optionally, you could dynamically expand storage here, but for simplicity, we'll just print an error
        printf("Device storage is full! Cannot add more devices.\n");
        exit(EXIT_FAILURE);  // Or handle it in another way
    }
}

