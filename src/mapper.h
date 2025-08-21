#ifndef GEMU_MAPPER_H
#define GEMU_MAPPER_H

#include "stdinc.h"
#include <stddef.h>

typedef struct Mapper Mapper;

/**
 * \brief Creates a mapper corresponding to the header information in the given
 * ROM data.
 *
 * This mapper must eventually be destroyed via Mapper_destroy.
 *
 * Will bail if rom is less than 0x8000 bytes long.
 *
 * \param rom borrowed ROM data to create the mapper according to.
 * \param rom_len length of rom.
 *
 * \return a pointer to the created Mapper, which must be destroyed via
 * Mapper_destroy.
 *
 * \sa Mapper_destroy
 */
Mapper *Mapper_from_rom(const u8 *rom, size_t rom_len);

/**
 * \brief Reads a byte from the given mapper.
 *
 * \param self the mapper to read from.
 * \param rom borrowed ROM data wired to the mapper.
 * \param rom_len length of rom.
 * \param addr the address to read from.
 *
 * \return the value at addr from the mapper.
 *
 * \sa Mapper_write
 */
u8 Mapper_read(const Mapper *self, const u8 *rom, size_t rom_len, u16 addr);

/**
 * \brief Writes a byte to the given mapper.
 *
 * \param self the mapper to write to.
 * \param addr the address to write to.
 * \param value the value to write.
 *
 * \sa Mapper_read
 */
void Mapper_write(Mapper *self, u16 addr, u8 value);

/**
 * \brief Cleans up any memory used by a mapper.
 *
 * \param self pointer to the mapper to destroy.
 *
 * \sa Mapper_from_rom
 */
void Mapper_destroy(Mapper *self);

#endif
