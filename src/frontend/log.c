#include "frontend/log.h"
#include <stdio.h>
#include "common/control.h"
#include "common/log.h"

static const char* level_label(const LogLevel level) {
    switch (level) {
        case LogLevel_INFO:
            return "\033[34mINFO";
        case LogLevel_WARN:
            return "\033[33mWARN";
        case LogLevel_ERROR:
            return "\033[31mERROR";
        default:
            bail("unreachable");
    }
}

void pretty_log(const LogLevel level, const LogCategory category, const char* const restrict text) {
    const char* const restrict label = level_label(level);
    FILE* const restrict stream = level == LogLevel_ERROR ? stderr : stdout;

    fprintf(stream, "\033[90m[%s\033[90m] (%d):\033[0m ", label, category);
    fputs(text, stream);
    fputc('\n', stream);
}
