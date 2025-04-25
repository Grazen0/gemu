#ifndef COMMON_CONTROL_H
#define COMMON_CONTROL_H

#include <stdbool.h>

void bail(const char* restrict format, ...);

void bail_if(bool cond, const char* restrict format, ...);

#endif
