#ifndef CORE_GAME_BOY_H
#define CORE_GAME_BOY_H

#include "cpu.h"
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
    LcdControl_Enable = 1 << 7,
    LcdControl_WinTileMap = 1 << 6,
    LcdControl_WinEnable = 1 << 5,
    LcdControl_BgwTileArea = 1 << 4,
    LcdControl_BgTileMap = 1 << 3,
    LcdControl_ObjSize = 1 << 2,
    LcdControl_ObjEnable = 1 << 1,
    LcdControl_ObjBgwEnable = 1 << 0,
} LcdControl;

typedef enum InterruptFlag : uint8_t {
    InterruptFlag_VBlank = 1 << 0,
    InterruptFlag_Lcd = 1 << 1,
    InterruptFlag_Timer = 1 << 2,
    InterruptFlag_Serial = 1 << 3,
    InterruptFlag_Joypad = 1 << 4,
} InterruptFlag;

typedef enum Joypad : uint8_t {
    Joypad_RightA = 1 << 0,
    Joypad_LeftB = 1 << 1,
    Joypad_UpSelect = 1 << 2,
    Joypad_DownStart = 1 << 3,
    Joypad_DPadSelect = 1 << 4,
    Joypad_ButtonsSelect = 1 << 5,
} Joypad;

typedef enum StatSelect : uint8_t {
    StatSelect_Mode0 = 1 << 3,
    StatSelect_Mode1 = 1 << 4,
    StatSelect_Mode2 = 1 << 5,
    StatSelect_Lyc = 1 << 6,
} StatSelect;

typedef enum ObjAttrs : uint8_t {
    ObjAttrs_Priority = 1 << 7,
    ObjAttrs_FlipY = 1 << 6,
    ObjAttrs_FlipX = 1 << 5,
    ObjAttrs_DmgPalette = 1 << 4,
    ObjAttrs_Bank = 1 << 3,
    ObjAttrs_CgbPalette = 0b111,
} ObjAttrs;

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
