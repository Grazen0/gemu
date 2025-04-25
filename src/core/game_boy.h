#ifndef CORE_GAME_BOY_H
#define CORE_GAME_BOY_H

#include <stddef.h>
#include <stdint.h>
#include "cpu.h"

extern const int GB_LCD_WIDTH;
extern const int GB_LCD_HEIGHT;
extern const int GB_BG_WIDTH;
extern const int GB_BG_HEIGHT;
extern const int GB_LCD_MAX_LY;
extern const int GB_CPU_FREQ;
extern const double GB_CPU_FREQ_M;

extern const uint8_t LCDC_ENABLE;
extern const uint8_t LCDC_WIN_TILE_MAP;
extern const uint8_t LCDC_WIN_ENABLE;
extern const uint8_t LCDC_BGW_TILE_AREA;
extern const uint8_t LCDC_BG_TILE_MAP;
extern const uint8_t LCDC_OBJ_SIZE;
extern const uint8_t LCDC_OBJ_ENABLE;
extern const uint8_t LCDC_OBJ_BGW_ENABLE;

typedef struct GameBoy {
    Cpu cpu;
    uint8_t ram[0x2000];
    uint8_t vram[0x2000];
    uint8_t hram[0x7F];
    uint8_t* rom;
    size_t rom_len;
    bool rom_enable;
    uint8_t lcdc;
    uint8_t lcds;
    uint8_t ly;
    uint8_t lcy;
    uint8_t scx;
    uint8_t scy;
    uint8_t wx;
    uint8_t wy;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t ie;
    uint8_t iff;
    uint8_t sb;
    uint8_t sc;
    uint8_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    uint8_t joyp;

} GameBoy;

GameBoy GameBoy_new(uint8_t* restrict rom, size_t rom_len);

void GameBoy_destroy(GameBoy* restrict gb);

uint8_t GameBoy_read_mem(const void* restrict ctx, uint16_t addr);

void GameBoy_write_mem(void* restrict ctx, uint16_t addr, uint8_t value);

#endif
