#ifndef GEMU_GAME_BOY_H
#define GEMU_GAME_BOY_H

#include "cpu.h"
#include "mapper.h"
#include <stddef.h>

static constexpr int GB_LCD_WIDTH = 160;
static constexpr int GB_LCD_HEIGHT = 144;
static constexpr int GB_BG_WIDTH = 256;
static constexpr int GB_BG_HEIGHT = 256;
static constexpr int GB_CLK_FREQ_HZ = 4'194'304;
static constexpr size_t GB_BOOT_ROM_LEN = 0x100;

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
    Mapper mapper;
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
    u8 joyp_prev;
    u8 stat_line;
    bool video_dirty;
    bool dma_pending;
    bool boot_rom_enable;
} GameBoy;

[[nodiscard]] GameBoy gb_init(Sink sink, const u8 *boot_rom);

void gb_deinit(GameBoy *gb);

void gb_load_rom(GameBoy *gb, const u8 *rom, size_t rom_len);

u64 gb_dispatch_cpu_instr(GameBoy *gb);

u64 gb_dispatch_pixel(GameBoy *gb);

u64 gb_dispatch_div(GameBoy *gb);

u64 gb_dispatch_tima(GameBoy *gb);

u64 gb_dispatch_dma_cp(GameBoy *gb);

#endif
