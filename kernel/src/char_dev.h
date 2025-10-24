#ifndef CHAR_DEV_H
#define CHAR_DEV_H

#include <stdint.h>

typedef struct CharDeviceOps {
    int (*read)(void *device, char* buffer, int size);
    int (*write)(void *device, const char* buffer, int size);
    int (*flush)(void *device);
} CharDeviceOps;

typedef struct CharDevice {
    CharDeviceOps* ops;
    CharDevice * private_data;

    uint32_t major;
    uint32_t minor;
    char name[32];
} CharDevice;


#endif