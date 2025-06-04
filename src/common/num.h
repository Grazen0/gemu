#ifndef LIB_NUM_H
#define LIB_NUM_H

#include <stdbool.h>
#include <stdint.h>

uint16_t concat_u16(uint8_t hi, uint8_t lo);

void set_bits(uint8_t* restrict dest, uint8_t mask, bool value);

#endif
