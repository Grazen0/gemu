#ifndef LIB_NUM_H
#define LIB_NUM_H

#include <stdint.h>

#define CONCAT_U16(hi, lo) (((uint16_t)hi << 8) | (uint16_t)lo)

#define HI(n) (n >> 8)
#define LO(n) (n & 0xFF)

#endif
