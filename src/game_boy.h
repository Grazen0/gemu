#ifndef GEMU_GAME_BOY_H
#define GEMU_GAME_BOY_H

#include "cpu.h"
#include <stddef.h>

constexpr int GB_LCD_WIDTH = 160;
constexpr int GB_LCD_HEIGHT = 144;
constexpr int GB_BG_WIDTH = 256;
constexpr int GB_BG_HEIGHT = 256;
constexpr int GB_LCD_MAX_LY = 154;
constexpr int GB_CPU_FREQUENCY_HZ = 4194304 / 4;
constexpr double GB_VBLANK_FREQ = 59.7;
constexpr size_t GB_BOOT_ROM_LEN = 0x100;

typedef enum : u8 {
    LcdControl_Enable = 1 << 7,
    LcdControl_WinTileMap = 1 << 6,
    LcdControl_WinEnable = 1 << 5,
    LcdControl_BgwTileArea = 1 << 4,
    LcdControl_BgTileMap = 1 << 3,
    LcdControl_ObjSize = 1 << 2,
    LcdControl_ObjEnable = 1 << 1,
    LcdControl_ObjBgwEnable = 1 << 0,
} LcdControl;

typedef enum : u8 {
    InterruptFlag_VBlank = 1 << 0,
    InterruptFlag_Lcd = 1 << 1,
    InterruptFlag_Timer = 1 << 2,
    InterruptFlag_Serial = 1 << 3,
    InterruptFlag_Joypad = 1 << 4,
} InterruptFlag;

typedef enum : u8 {
    Joypad_RightA = 1 << 0,
    Joypad_LeftB = 1 << 1,
    Joypad_UpSelect = 1 << 2,
    Joypad_DownStart = 1 << 3,
    Joypad_DPadSelect = 1 << 4,
    Joypad_ButtonsSelect = 1 << 5,
} Joypad;

typedef enum : u8 {
    StatSelect_Mode0 = 1 << 3,
    StatSelect_Mode1 = 1 << 4,
    StatSelect_Mode2 = 1 << 5,
    StatSelect_Lyc = 1 << 6,
} StatSelect;

typedef enum : u8 {
    ObjAttrs_Priority = 1 << 7,
    ObjAttrs_FlipY = 1 << 6,
    ObjAttrs_FlipX = 1 << 5,
    ObjAttrs_DmgPalette = 1 << 4,
    ObjAttrs_Bank = 1 << 3,
    ObjAttrs_CgbPalette = 0b111,
} ObjAttrs;

typedef struct {
    bool up;
    bool down;
    bool right;
    bool left;
    bool a;
    bool b;
    bool start;
    bool select;
} JoypadState;

typedef struct {
    JoypadState joypad;
    Cpu cpu;
    bool boot_rom_exists;
    bool boot_rom_enable;
    u8 ram[0x2000];
    u8 vram[0x2000];
    u8 hram[0x7F];
    u8 oam[0xA0];
    u8 boot_rom[GB_BOOT_ROM_LEN];
    u8 *rom;
    size_t rom_len;
    u8 lcdc;
    u8 stat;
    u8 ly;
    u8 lcy;
    u8 scx;
    u8 scy;
    u8 wx;
    u8 wy;
    u8 bgp;
    u8 obp0;
    u8 obp1;
    u8 ie;
    u8 if_;
    u8 sb;
    u8 sc;
    u8 div;
    u8 tima;
    u8 tma;
    u8 tac;
    u8 joyp;
} GameBoy;

/**
 * \brief Constructs a GameBoy object with the given boot ROM.
 *
 * Constructs a GameBoy with the given boot ROM (which may be NULL). The created
 * GameBoy must eventually be destroyed with GameBoy_destroy.
 *
 * The data at boot_rom is copied, so ownership of boot_rom is not taken.
 *
 * \param boot_rom the boot ROM to use. **Must** be either NULL or exactly 256
 * bytes long.
 *
 * \return the constructed GameBoy.
 *
 * \sa GameBoy_destroy
 */
[[nodiscard]] GameBoy GameBoy_new(const u8 *boot_rom);

/**
 * \brief Frees a previously-created GameBoy.
 *
 * \param self the GameBoy to destruct.
 *
 * \sa GameBoy_new
 */
void GameBoy_destroy(GameBoy *self);

/**
 * \brief Logs information about the currently loaded ROM.
 *
 * If no ROM is currently loaded, this will be a no-op.
 *
 * \param self the GameBoy to log information about.
 *
 * \sa GameBoy_load_rom
 */
void GameBoy_log_cartridge_info(const GameBoy *self);

/**
 * \brief Loads ROM data into a GameBoy.
 *
 * This method copies rom, so it does not take ownership of it.
 *
 * \param self the GameBoy to load the ROM to.
 * \param rom the ROM data to load.
 * \param rom_len the length of rom.
 */
void GameBoy_load_rom(GameBoy *self, const u8 *rom, size_t rom_len);

[[nodiscard]] u8 GameBoy_read_mem(const void *ctx, u16 addr);

void GameBoy_write_mem(void *ctx, u16 addr, u8 value);

void GameBoy_service_interrupts(GameBoy *self, Memory *mem);

#endif
