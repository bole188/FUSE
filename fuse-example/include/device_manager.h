#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H
#define MAX_NAME_LENGTH 50
#define MAX_MODEL_LENGTH 20
#define INITIAL_CAPACITY 10
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Enum for entry type
typedef enum {
    FILE_TYPE,
    FOLDER_TYPE
} EntryType;

// DeviceEntry structure
typedef struct {
    char name[100];
    char model[20];
    int serial_number;
    time_t registration_date;
    int system_id;
    char imei[8];
    EntryType type;
} DeviceEntry;

#define MAX_DEVICES 100
extern DeviceEntry* device_storage;
extern int device_count;
extern int device_capacity;

// Function prototypes
void ensure_device_capacity();
DeviceEntry *create_and_add_device_entry(const char *name, const char *model, 
                                 int serial_number, time_t registration_date, 
                                 int imei, EntryType type);
int is_valid_model(const char *model, EntryType type);

#endif // DEVICE_MANAGER_H
