#pragma once
/* Pull in the compiler's freestanding limits (INT_MAX, LONG_MAX, ...) then add
 * the POSIX bits doom expects. */
#include_next <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
