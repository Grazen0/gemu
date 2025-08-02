#ifndef GEMU_NUM_H
#define GEMU_NUM_H

#include "stdinc.h"

[[nodiscard]] u16 concat_u16(u8 hi, u8 lo);

void set_bits(u8 * dest, u8 mask, bool value);

#endif
