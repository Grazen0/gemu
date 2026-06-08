#ifndef GEMU_GAME_BOY_H
#define GEMU_GAME_BOY_H

#include "cpu.h"
#include "data.h"
#include <stddef.h>

static constexpr int GB_LCD_WIDTH = 160;
static constexpr int GB_LCD_HEIGHT = 144;
static constexpr int GB_BG_WIDTH = 256;
static constexpr int GB_BG_HEIGHT = 256;
static constexpr int GB_CLK_FREQ_HZ = 4'194'304;
static constexpr size_t GB_BOOT_ROM_LEN = 0x100;

typedef enum : u8 {
    LCDC_ENABLE = 1 << 7,
    LCDC_WIN_TILE_MAP = 1 << 6,
    LCDC_WIN_ENABLE = 1 << 5,
    LCDC_BG_WIN_TILES = 1 << 4,
    LCDC_BG_TILE_MAP = 1 << 3,
    LCDC_OBJ_SIZE = 1 << 2,
    LCDC_OBJ_ENABLE = 1 << 1,
    LcdControl_ObjBgwEnable = 1 << 0,
} LcdControl;

typedef enum : u8 {
    INT_VBLANK = 1 << 0,
    INT_LCD = 1 << 1,
    INT_TIMER = 1 << 2,
    INT_SERIAL = 1 << 3,
    INT_JOYPAd = 1 << 4,
} InterruptFlag;

typedef enum : u8 {
    JOYP_RIGHT_A = 1 << 0,
    JOPY_LEFT_B = 1 << 1,
    JOYP_UP_SELECT = 1 << 2,
    JOYP_DOWN_START = 1 << 3,
    JOYP_SELECT_DPAD = 1 << 4,
    JOYP_SELECT_BUTTONS = 1 << 5,
} Joypad;

typedef enum : u8 {
    STAT_PPU_MODE = 0b11,
    STAT_LCY_EQ_LY = 1 << 2,
    STAT_MODE0_INT = 1 << 3,
    STAT_MODE1_INT = 1 << 4,
    STAT_MODE2_INT = 1 << 5,
    STAT_LYC_INT = 1 << 6,
} StatSelect;

typedef enum : u8 {
    OBJ_ATTRS_CGB_PALETTE = 0b111,
    OBJ_ATTRS_ANK = 1 << 3,
    OBJ_ATTRS_DMG_PALETTE = 1 << 4,
    OBJ_ATTRS_FLIP_X = 1 << 5,
    OBJ_ATTRS_FLIP_Y = 1 << 6,
    OBJ_ATTRS_PRIORITY = 1 << 7,
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
} JoypadButtons;

typedef struct {
    Cpu cpu;
    JoypadButtons btns;
    u8 (*render_buf)[GB_BG_WIDTH];
    u8 (*scanout_buf)[GB_LCD_WIDTH];
    u8 *rom;
    size_t rom_len;
    u16 dma_cur_addr;
    u8 *ram;
    u8 *vram;
    u8 *hram;
    u8 *oam;
    u8 *boot_rom;
    u8 lcdc;
    u8 stat;
    u8 ly;
    u16 lx;
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
    bool video_dirty;
    bool dma_pending;
    bool boot_rom_enable;
} GameBoy;

typedef struct {
    char title[17];
    CartridgeType cart_type;
    u8 ram_size;
    u8 rom_size;
} GameInfo;

[[nodiscard]] GameBoy gb_init(const u8 *boot_rom);

void gb_deinit(GameBoy *gb);

[[nodiscard]] GameInfo gb_cartridge_info(const u8 *rom);

void gb_load_rom(GameBoy *gb, const u8 *rom, size_t rom_len);

[[nodiscard]] u8 gb_read_mem(const GameBoy *gbx, u16 addr);

void gb_write_mem(GameBoy *gb, u16 addr, u8 value);

void gb_service_interrupts(GameBoy *gb, Memory *mem);

u64 gb_dispatch_cpu_instr(GameBoy *gb);

u64 gb_dispatch_pixel(GameBoy *gb);

u64 gb_dispatch_div(GameBoy *gb);

u64 gb_dispatch_tima(GameBoy *gb);

u64 gb_dispatch_dma_cp(GameBoy *gb);

#endif
