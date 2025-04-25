#include "data.h"
#include <stdlib.h>
#include <string.h>

const size_t ROM_ENTRY_POINT = 0x100;
const size_t ROM_NINTENDO_LOGO = 0x104;
const size_t ROM_TITLE = 0x134;
const size_t ROM_MANUFACTURER_CODE = 0x13F;
const size_t ROM_CGB_FLAG = 0x143;
const size_t ROM_NEW_LICENSEE_CODE = 0x144;
const size_t ROM_SGB_FLAG = 0x146;
const size_t ROM_CARTRIDGE_TYPE = 0x147;
const size_t ROM_ROM_SIZE = 0x148;
const size_t ROM_RAM_SIZE = 0x149;
const size_t ROM_DESTINATION_CODE = 0x14A;
const size_t ROM_OLD_LICENSEE_CODE = 0x14B;
const size_t ROM_MASK_ROM_VERSION_NUMBER = 0x14C;
const size_t ROM_HEADER_CHECKSUM = 0x14D;
const size_t ROM_GLOBAL_CHECKSUM = 0x14E;

char* get_rom_title(uint8_t* rom) {
    size_t title_len = 0;

    while (title_len < 0x10 && rom[ROM_TITLE + title_len] != '\0') {
        title_len++;
    }

    char* buffer = malloc(title_len * sizeof(*buffer));
    memcpy(buffer, rom + ROM_TITLE, title_len);
    return buffer;
}
