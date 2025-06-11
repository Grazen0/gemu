
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void bail(const char* const format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(1);
}

void bail_if(const bool condition, const char* const format, ...) {
    va_list args;
    va_start(args, format);

    if (condition) {
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
        va_end(args);
        exit(1);
    }

    va_end(args);
}
