#pragma once

/* Created in March 24, 2026.
 *
 * This setup/kernel params system is based on code i have seen on old versions
 * of linux kernel < 3.0. The system consists of basically of the struct SetupEntry which has a function
 * as a callback and a label for it, the macro __setup is responsible for registering the setup function by
 * creating a static global variable with the prefix __setup_ and applying the attribute
 * section(".kernel_params") to ensure that the location of this data is within a specific section of the
 * binary.
 *
 * As the entries are used just on boot, the isolation of theese memory pages makes the pages easily freeable
 * and reclaimable to the system. */

#include <types.h>

typedef int (*setup_fn)(char *);

typedef struct SetupEntry {
    const char *str;
    setup_fn fn;
} SetupEntry;

#define __setup(name, fn)                                                                                    \
    static const SetupEntry __setup_##fn __attribute__((section(".kernel_params"), used)) = {name, fn}
