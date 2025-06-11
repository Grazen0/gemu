#ifndef COMMON_CONTROL_H
#define COMMON_CONTROL_H

[[noreturn]] void bail(const char* format, ...);

void bail_if(bool condition, const char* format, ...);

#endif
