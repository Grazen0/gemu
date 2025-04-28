#include "data.h"
#include <stdlib.h>
#include <string.h>

char* get_rom_title(uint8_t* rom) {
    size_t title_len = 0;

    while (title_len < 0x10 && rom[ROM_TITLE + title_len] != '\0') {
        title_len++;
    }

    char* buffer = malloc(title_len * sizeof(*buffer));
    memcpy(buffer, rom + ROM_TITLE, title_len);
    return buffer;
}
