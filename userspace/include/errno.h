#pragma once
#include <abi/errno.h>

#ifndef EISDIR
#define EISDIR 21
#endif
#ifndef ERANGE
#define ERANGE 34
#endif
#ifndef EEXIST
#define EEXIST 17
#endif

extern int errno;
