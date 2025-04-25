#include "common/control.h"
#include <stdarg.h>
#include <stdlib.h>
#include "common/log.h"

void bail(const char* const restrict format, ...) {
    va_list args;
    va_start(args, format);
    vlog_error(format, args);
    va_end(args);
    exit(1);
}

void bail_if(const bool cond, const char* const restrict format, ...) {
    va_list args;
    va_start(args, format);
    if (cond) {
        vlog_error(format, args);
        va_end(args);
        exit(1);
    }
    va_end(args);
}
