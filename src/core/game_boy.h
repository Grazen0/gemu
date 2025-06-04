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
extern const int GB_CPU_FREQUENCY_HZ;
extern const double GB_VBLANK_FREQ;

typedef enum LcdControl {
    LcdControl_ENABLE = 1 << 7,
    LcdControl_WIN_TILE_MAP = 1 << 6,
    LcdControl_WIN_ENABLE = 1 << 5,
    LcdControl_BGW_TILE_AREA = 1 << 4,
    LcdControl_BG_TILE_MAP = 1 << 3,
    LcdControl_OBJ_SIZE = 1 << 2,
    LcdControl_OBJ_ENABLE = 1 << 1,
    LcdControl_OBJ_BGW_ENABLE = 1 << 0,
} LcdControl;

typedef enum InterruptFlag {
    InterruptFlag_VBLANK = 1 << 0,
    InterruptFlag_LCD = 1 << 1,
    InterruptFlag_TIMER = 1 << 2,
    InterruptFlag_SERIAL = 1 << 3,
    InterruptFlag_JOYPAD = 1 << 4,
} InterruptFlag;

typedef struct GameBoy {
    Cpu cpu;
    uint8_t ram[0x2000];
    uint8_t vram[0x2000];
    uint8_t hram[0x7F];
    uint8_t* boot_rom;
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
    uint8_t if_;
    uint8_t sb;
    uint8_t sc;
    uint8_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    uint8_t joyp;

} GameBoy;

GameBoy GameBoy_new(uint8_t* boot_rom, uint8_t* rom, size_t rom_len);

void GameBoy_destroy(GameBoy* restrict gb);

uint8_t GameBoy_read_mem(const void* restrict ctx, uint16_t addr);

void GameBoy_write_mem(void* restrict ctx, uint16_t addr, uint8_t value);

void GameBoy_service_interrupts(GameBoy* restrict gb, Memory* restrict mem);

#endif
