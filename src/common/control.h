#ifndef COMMON_CONTROL_H
#define COMMON_CONTROL_H

#include <stdbool.h>
#include "sys/cdefs.h"

void bail(const char* restrict format, ...) __THROW __attribute__((__noreturn__));

void bail_if(bool cond, const char* restrict format, ...);

#endif
