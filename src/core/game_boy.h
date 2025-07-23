#ifndef CORE_GAME_BOY_H
#define CORE_GAME_BOY_H

#include "cpu/cpu.h"
#include <stddef.h>
#include <stdint.h>

constexpr int GB_LCD_WIDTH = 160;
constexpr int GB_LCD_HEIGHT = 144;
constexpr int GB_BG_WIDTH = 256;
constexpr int GB_BG_HEIGHT = 256;
constexpr int GB_LCD_MAX_LY = 154;
constexpr int GB_CPU_FREQUENCY_HZ = 4194304 / 4;
constexpr double GB_VBLANK_FREQ = 59.7;

typedef enum LcdControl : uint8_t {
    LcdControl_ENABLE = 1 << 7,
    LcdControl_WIN_TILE_MAP = 1 << 6,
    LcdControl_WIN_ENABLE = 1 << 5,
    LcdControl_BGW_TILE_AREA = 1 << 4,
    LcdControl_BG_TILE_MAP = 1 << 3,
    LcdControl_OBJ_SIZE = 1 << 2,
    LcdControl_OBJ_ENABLE = 1 << 1,
    LcdControl_OBJ_BGW_ENABLE = 1 << 0,
} LcdControl;

typedef enum InterruptFlag : uint8_t {
    InterruptFlag_VBLANK = 1 << 0,
    InterruptFlag_LCD = 1 << 1,
    InterruptFlag_TIMER = 1 << 2,
    InterruptFlag_SERIAL = 1 << 3,
    InterruptFlag_JOYPAD = 1 << 4,
} InterruptFlag;

typedef enum Joypad : uint8_t {
    Joypad_A_RIGHT = 1 << 0,
    Joypad_B_LEFT = 1 << 1,
    Joypad_SELECT_UP = 1 << 2,
    Joypad_START_DOWN = 1 << 3,
    Joypad_D_PAD_SELECT = 1 << 4,
    Joypad_BUTTONS_SELECT = 1 << 5,
} Joypad;

typedef enum StatSelect : uint8_t {
    StatSelect_MODE_0 = 1 << 3,
    StatSelect_MODE_1 = 1 << 4,
    StatSelect_MODE_2 = 1 << 5,
    StatSelect_LYC = 1 << 6,
} StatSelect;

typedef struct JoypadState {
    bool up;
    bool down;
    bool right;
    bool left;
    bool a;
    bool b;
    bool start;
    bool select;
} JoypadState;

typedef struct GameBoy {
    JoypadState joypad;
    Cpu cpu;
    uint8_t ram[0x2000];
    uint8_t vram[0x2000];
    uint8_t hram[0x7F];
    uint8_t oam[0xA0];
    uint8_t* boot_rom;
    uint8_t* rom;
    size_t rom_len;
    bool rom_enable;
    uint8_t lcdc;
    uint8_t stat;
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

[[nodiscard]] GameBoy GameBoy_new(uint8_t* boot_rom, uint8_t* rom, size_t rom_len);

void GameBoy_destroy(GameBoy* restrict gb);

[[nodiscard]] uint8_t GameBoy_read_mem(const void* restrict ctx, uint16_t addr);

void GameBoy_write_mem(void* restrict ctx, uint16_t addr, uint8_t value);

void GameBoy_service_interrupts(GameBoy* restrict gb, Memory* restrict mem);

#endif
