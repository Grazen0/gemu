#include "common/num.h"
#include <stdint.h>

uint16_t concat_u16(const uint8_t hi, const uint8_t lo) {
    return ((uint16_t)hi << 8) | (uint16_t)lo;
}
