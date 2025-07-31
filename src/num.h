#ifndef GEMU_NUM_H
#define GEMU_NUM_H

#include <stdint.h>

[[nodiscard]] uint16_t concat_u16(uint8_t hi, uint8_t lo);

void set_bits(uint8_t* restrict dest, uint8_t mask, bool value);

#endif
