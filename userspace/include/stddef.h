#pragma once

typedef unsigned long size_t;
typedef long          ssize_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(t, m) __builtin_offsetof(t, m)
