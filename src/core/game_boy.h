#ifndef CORE_GAME_BOY_H
#define CORE_GAME_BOY_H

#include <stddef.h>
#include <stdint.h>
#include "cpu.h"
#include "sys/types.h"

#define GB_LCD_WIDTH 160
#define GB_LCD_HEIGHT 144
#define GB_BG_WIDTH 256
#define GB_BG_HEIGHT 256
#define GB_LCD_MAX_LY 154
#define GB_CPU_FREQ 4194304  // Hz
#define GB_CPU_FREQ_M ((double)GB_CPU_FREQ / 4.0)

#define LCDC_ENABLE (1 << 7)
#define LCDC_WIN_TILE_MAP (1 << 6)
#define LCDC_WIN_ENABLE (1 << 5)
#define LCDC_BGW_TILE_AREA (1 << 4)
#define LCDC_BG_TILE_MAP (1 << 3)
#define LCDC_OBJ_SIZE (1 << 2)
#define LCDC_OBJ_ENABLE (1 << 1)
#define LCDC_OBJ_BGW_ENABLE (1 << 0)

typedef struct GameBoy {
    Cpu cpu;
    int cycle_count;
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

uint8_t GameBoy_read_mem(GameBoy* restrict gb, u_int16_t addr);

uint16_t GameBoy_read_mem_u16(GameBoy* restrict gb, u_int16_t addr);

void GameBoy_write_mem(GameBoy* restrict gb, u_int16_t addr, uint8_t value);

void GameBoy_write_mem_u16(GameBoy* restrict gb, u_int16_t addr, uint16_t value);

uint8_t GameBoy_read_pc(GameBoy* restrict gb);

uint16_t GameBoy_read_pc_u16(GameBoy* restrict gb);

void GameBoy_stack_push_u16(GameBoy* restrict gb, uint16_t value);

uint16_t GameBoy_stack_pop_u16(GameBoy* restrict gb);

uint8_t GameBoy_read_r(GameBoy* restrict gb, CpuTableR r);

void GameBoy_write_r(GameBoy* restrict gb, CpuTableR r, uint8_t value);

void GameBoy_tick(GameBoy* restrict gb);

void GameBoy_execute(GameBoy* restrict gb, uint8_t opcode);

void GameBoy_execute_prefixed(GameBoy* restrict gb, uint8_t opcode);

void GameBoy_interrupt(GameBoy* restrict gb, uint8_t handler_location);

#endif
