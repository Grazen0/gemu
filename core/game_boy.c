#include "game_boy.h"
#include "cpu.h"
#include "data.h"
#include "log.h"
#include "macros.h"
#include "mapper.h"
#include "string.h"
#include "util.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr size_t RAM_SIZE = 0x2000;
static constexpr size_t VRAM_SIZE = 0x2000;
static constexpr size_t HRAM_SIZE = 0x7F;
static constexpr size_t OAM_SIZE = 0xA0;

static void gb_update_joyp(GameBoy *gb)
{
    gb->joyp_prev = gb->joyp;
    gb->joyp |= 0x0F;

    if ((gb->joyp & JOYP_SELECT_DPAD) == 0) {
        if (gb->btns.right)
            gb->joyp &= ~JOYP_RIGHT_A;

        if (gb->btns.left)
            gb->joyp &= ~JOPY_LEFT_B;

        if (gb->btns.up)
            gb->joyp &= ~JOYP_UP_SELECT;

        if (gb->btns.down)
            gb->joyp &= ~JOYP_DOWN_START;
    }

    if ((gb->joyp & JOYP_SELECT_BUTTONS) == 0) {
        if (gb->btns.a)
            gb->joyp &= ~JOYP_RIGHT_A;

        if (gb->btns.b)
            gb->joyp &= ~JOPY_LEFT_B;

        if (gb->btns.select)
            gb->joyp &= ~JOYP_UP_SELECT;

        if (gb->btns.start)
            gb->joyp &= ~JOYP_DOWN_START;
    }
}

// https://gbdev.io/pandocs/The_Cartridge_Header.html#014d--header-checksum
static void verify_rom_checksum(const u8 *rom)
{
    u8 chksm = 0;
    for (u16 addr = 0x0134; addr <= 0x014C; ++addr)
        chksm = chksm - rom[addr] - 1;

    if (chksm != rom[ROM_HEADER_CHECKSUM]) {
        BAIL("ROM checksum mismatch (expected $%02X, was $%02X)",
             rom[ROM_HEADER_CHECKSUM], chksm);
    }
}

static void gb_mock_boot(GameBoy *gb)
{
    verify_rom_checksum(gb->rom);

    gb->cpu.a = 0x01;
    gb->cpu.b = 0x00;
    gb->cpu.c = 0x13;
    gb->cpu.d = 0x00;
    gb->cpu.e = 0xD8;
    gb->cpu.h = 0x01;
    gb->cpu.l = 0x4D;
    gb->cpu.sp = 0xFFFE;
    gb->cpu.pc = 0x0100;

    gb->boot_rom_enable = false;
}

static void gb_reset(GameBoy *gb)
{
    gb->cpu.pc = 0;
    gb->boot_rom_enable = true;
}

static void gb_validate_rom(const GameBoy *gb)
{
    if (!cart_type_has_ram(gb->rom[ROM_HEADER_CART_TYPE]) &&
        gb->rom[ROM_HEADER_RAM_SIZE] != 0) {
        BAIL(
            "Cartridge type does not have RAM, but header indicates otherwise (cartridge type: $%02X, RAM size: $%02X)",
            gb->rom[ROM_HEADER_CART_TYPE], gb->rom[ROM_HEADER_RAM_SIZE]);
    }

    if (gb->rom_len != 0x8000 * ((size_t)1 << gb->rom[ROM_HEADER_ROM_SIZE])) {
        BAIL(
            "Actual ROM size does not match header-specified size. (specified: %u, was: %zu)",
            gb->rom[ROM_HEADER_ROM_SIZE], gb->rom_len);
    }
}

static JoypadButtons joypad_btns_init()
{
    return (JoypadButtons){
        .up = false,
        .down = false,
        .right = false,
        .left = false,
        .a = false,
        .b = false,
        .start = false,
        .select = false,
    };
}

GameBoy gb_init(Logger logger, const u8 *boot_rom)
{
    GameBoy gb = {
        .cpu = cpu_init(logger),
        .mapper = mapper_default(),
        .ram = nullptr,
        .vram = nullptr,
        .boot_rom = nullptr,
        .hram = nullptr,
        .oam = nullptr,
        .btns = joypad_btns_init(),
        .render_buf = nullptr,
        .scanout_buf = nullptr,
        .rom = nullptr,
        .rom_len = 0,
        .dma_cur_addr = 0,
        .boot_rom_enable = true,
        .lcdc = 0,
        .stat = 0,
        .ly = 0,
        .lx = 0,
        .lcy = 0,
        .scx = 0,
        .scy = 0,
        .wx = 0,
        .wy = 0,
        .bgp = 0,
        .obp0 = 0,
        .obp1 = 0,
        .ie = 0,
        .if_ = 0,
        .sb = 0,
        .sc = 0,
        .div = 0,
        .tima = 0,
        .tma = 0,
        .tac = 0,
        .joyp = 0x0F,
        .joyp_prev = 0x0F,
        .dma_pending = false,
    };

    gb.ram = calloc(RAM_SIZE, sizeof(*gb.ram));
    assert(gb.ram != nullptr);

    gb.vram = calloc(VRAM_SIZE, sizeof(*gb.vram));
    assert(gb.vram != nullptr);

    gb.render_buf = calloc(GB_BG_HEIGHT, sizeof(*gb.render_buf));
    assert(gb.render_buf != nullptr);

    gb.scanout_buf = calloc(GB_LCD_HEIGHT, sizeof(*gb.scanout_buf));
    assert(gb.scanout_buf != nullptr);

    if (boot_rom != nullptr) {
        gb.boot_rom = calloc(GB_BOOT_ROM_LEN, sizeof(*gb.boot_rom));
        assert(gb.boot_rom != nullptr);

        memcpy(gb.boot_rom, boot_rom, GB_BOOT_ROM_LEN * sizeof(*gb.boot_rom));
    }

    gb.hram = calloc(HRAM_SIZE, sizeof(*gb.hram));
    assert(gb.hram != nullptr);

    gb.oam = calloc(OAM_SIZE, sizeof(*gb.oam));
    assert(gb.oam != nullptr);

    return gb;
}

void gb_deinit(GameBoy *gb)
{
    free(gb->rom);
    gb->rom = nullptr;
    gb->rom_len = 0;

    free(gb->boot_rom);
    gb->boot_rom = nullptr;

    free(gb->ram);
    gb->ram = nullptr;

    free(gb->vram);
    gb->vram = nullptr;

    free(gb->render_buf);
    gb->render_buf = nullptr;

    free(gb->scanout_buf);
    gb->scanout_buf = nullptr;

    free(gb->hram);
    gb->hram = nullptr;

    free(gb->oam);
    gb->oam = nullptr;

    mapper_deinit(&gb->mapper);
}

GameInfo gb_cartridge_info(const u8 *rom)
{
    GameInfo out = {
        .title = {},
        .cart_type = rom[ROM_HEADER_CART_TYPE],
        .ram_size = rom[ROM_HEADER_RAM_SIZE],
        .rom_size = rom[ROM_HEADER_ROM_SIZE],
    };

    memcpy(out.title, (char *)&rom[ROM_HEADER_TITLE], sizeof(out.title));
    return out;
}

void gb_load_rom(GameBoy *gb, const u8 *rom, size_t rom_len)
{
    if (rom_len < 0x8000) {
        BAIL("ROM data cannot be less than 32768 bytes long (was %zu)",
             rom_len);
    }

    free(gb->rom);
    gb->rom = calloc(rom_len, sizeof(*gb->rom));
    assert(gb->rom != nullptr);

    memcpy(gb->rom, rom, rom_len * sizeof(*gb->rom));
    gb->rom_len = rom_len;

    gb_validate_rom(gb);

    mapper_deinit(&gb->mapper);
    gb->mapper = mapper_from_rom(rom, rom_len);

    gb_reset(gb);

    if (gb->boot_rom == nullptr)
        gb_mock_boot(gb);
}

// NOLINTNEXTLINE
u8 gb_read_io(GameBoy *gb, u16 addr)
{
    if (addr == 0xFF00) // FF00 (joypad input)
        return gb->joyp;

    // TODO: implement serial transfer
    if (addr == 0xFF01) // FF01 (serial transfer data)
        return 0xFF;

    if (addr == 0xFF02) // FF02 (serial transfer control)
        return gb->sc;

    if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)

        // clang-format off
        switch (addr) {
            case 0xFF04: return gb->div;
            case 0xFF05: return gb->tima;
            case 0xFF06: return gb->tma;
            case 0xFF07: return gb->tac;
            default: BAIL("Unexpected I/O timer and divider read ($%04X)", addr);
        }
        // clang-format on
    }

    if (addr == 0xFF0F) // FF0F (interrupts)
        return gb->if_;

    if (addr >= 0xFF10 && addr <= 0xFF26) // FF10-FF26 (audio)
        BAIL("I/O audio read ($%04X)", addr);

    if (addr >= 0xFF30 && addr <= 0xFF3F) // FF30-FF3F (wave pattern)
        BAIL("I/O wave pattern read ($%04X)", addr);

    if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        // clang-format off
        switch (addr) {
            case 0xFF40: return gb->lcdc;
            case 0xFF44: return gb->ly;
            case 0xFF45: return gb->lcy;
            case 0xFF41: return gb->stat;
            case 0xFF42: return gb->scy;
            case 0xFF43: return gb->scx;
            case 0xFF4A: return gb->wy;
            case 0xFF4B: return gb->wx;
            case 0xFF47: return gb->bgp;
            case 0xFF48: return gb->obp0;
            case 0xFF49: return gb->obp1;
            default: BAIL("Unexpected I/O LCD read (addr = $%04X)", addr);
        }
        // clang-format on
    }

    if (addr == 0xFF4D) // FF4D (CGB registers, CGB-only)
        return 0xFF;

    if (addr == 0xFF4F) // FF4F (VRAM bank select, CGB-only)
        return 0xFF;

    if (addr == 0xFF50) // FF50 (boot ROM disable)
        return 0xFF;

    if (addr >= 0xFF51 && addr <= 0xFF55) // FF51-FF55 (VRAM DMA, CGB-only)
        return 0xFF;

    if (addr >= 0xFF68 && addr <= 0xFF6B) // FF68-FF6B (LCD palettes, CGB-only)
        return 0xFF;

    if (addr == 0xFF70) // FF70 (WRAM bank select, CGB-only)
        return 0xFF;

    BAIL("Unexpected I/O read (addr = $%04X)", addr);
}

static u8 gb_read_mem_0000_8000(GameBoy *gb, u16 addr)
{
    if (gb->boot_rom_enable && addr <= GB_BOOT_ROM_LEN) {
        // 0000-0100 (Boot ROM)
        if (gb->boot_rom == nullptr)
            BAIL("Tried to read non-existing boot ROM");

        return gb->boot_rom[addr];
    }

    // 0000-7FFF (from cartridge)
    return mapper_read(&gb->mapper, gb->rom, gb->rom_len, addr);
}

static u8 gb_read_mem_8000_A000(GameBoy *gb, u16 addr)
{
    // 8000-9FFF (VRAM)
    return gb->vram[addr - 0x8000];
}

static u8 gb_read_mem_A000_C000(GameBoy *gb, u16 addr)
{
    // A000-BFFF (External RAM)
    return mapper_read(&gb->mapper, gb->rom, gb->rom_len, addr);
}

static u8 gb_read_mem_C000_E000(GameBoy *gb, u16 addr)
{
    // C000-DFFF (WRAM)
    return gb->ram[addr - 0xC000];
}

static u8 gb_read_mem_E000_10000(GameBoy *gb, u16 addr)
{
    if (addr <= 0xFDFF) // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        return gb->ram[addr - 0xE000];

    if (addr <= 0xFE9F) // FE00-FE9F (OAM)
        return gb->oam[addr - 0xFE00];

    if (addr <= 0xFEFF) // FEA0-FEFF (Not usable)
        BAIL("Tried to read unusable memory (addr = $%04X)", addr);

    if (addr <= 0xFF7F) // FF00-FF7F (I/O registers)
        return gb_read_io(gb, addr);

    if (addr <= 0xFFFE) // FF80-FFFE (High RAM)
        return gb->hram[addr - 0xFF80];

    // FFFF (Interrupt Enable Register)
    return gb->ie;
}

static u8 gb_read_mem(GameBoy *gb, u16 addr)
{
    static u8 (*const HANDLERS[])(GameBoy *, u16) = {
        [0x0] = gb_read_mem_0000_8000,  [0x1] = gb_read_mem_0000_8000,
        [0x2] = gb_read_mem_0000_8000,  [0x3] = gb_read_mem_0000_8000,
        [0x4] = gb_read_mem_0000_8000,  [0x5] = gb_read_mem_0000_8000,
        [0x6] = gb_read_mem_0000_8000,  [0x7] = gb_read_mem_0000_8000,
        [0x8] = gb_read_mem_8000_A000,  [0x9] = gb_read_mem_8000_A000,
        [0xA] = gb_read_mem_A000_C000,  [0xB] = gb_read_mem_A000_C000,
        [0xC] = gb_read_mem_C000_E000,  [0xD] = gb_read_mem_C000_E000,
        [0xE] = gb_read_mem_E000_10000, [0xF] = gb_read_mem_E000_10000,
    };
    static_assert(ARRAY_LEN(HANDLERS) == 16);

    u8 nib = (addr >> 12) & 0xF;
    return HANDLERS[nib](gb, addr);
}

u16 gb_read_mem_u16(GameBoy *gb, u16 addr)
{
    u8 lo = gb_read_mem(gb, addr);
    u8 hi = gb_read_mem(gb, addr + 1);
    return concat_u16(hi, lo);
}

// NOLINTNEXTLINE
void gb_write_io(GameBoy *gb, u16 addr, u8 value)
{
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        // joyp will be loaded with gb->btns on the next read
        gb->joyp = value | 0x0F;
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        gb->sb = value;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        // TODO: implement properly
        gb->sc = value;
    } else if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)
        // clang-format off
        switch (addr) {
            case 0xFF04: gb->div = 0; break;
            case 0xFF05: gb->tima = value; break;
            case 0xFF06: gb->tma = value; break;
            case 0xFF07: gb->tac = value; break;
            default: BAIL("Unexpected I/O timer and divider write ($%04X, $%02X)", addr, value);
        }
        // clang-format on
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        gb->if_ = value;
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        // TODO: I/O audio write
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        // TODO: I/O wave pattern write
    } else if (addr == 0xFF46) {
        // FF46 (OAM DMA source address and start)
        assert(!gb->dma_pending);

        gb->dma_cur_addr = (u16)value << 8;
        gb->dma_pending = true;
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        // clang-format off
        switch (addr) {
            case 0xFF40: gb->lcdc = value; break;
            case 0xFF45: gb->lcy = value; break;
            case 0xFF41:
                // Modifies only bits 3-7
                gb->stat = (gb->stat & 0b111) | (value & ~0b111);
                break;
            case 0xFF42: gb->scy = value; break;
            case 0xFF43: gb->scx = value; break;
            case 0xFF44: break;
            case 0xFF4A: gb->wy = value; break;
            case 0xFF4B: gb->wx = value; break;
            case 0xFF47: gb->bgp = value; break;
            case 0xFF48: gb->obp0 = value; break;
            case 0xFF49: gb->obp1 = value; break;
            default: BAIL("Unexpected I/O LCD write (addr = $%04X, value = $%02X)", addr, value);
        }
        // clang-format on

        gb->video_dirty = true;
    } else if (addr == 0xFF4F) {
        // FF4F
        log_warn("I/O VRAM bank select write ($%04X, $%02X)", addr, value);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        if (value != 0)
            gb->boot_rom_enable = false;
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA, CGB-only)
        gb->video_dirty = true;
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (LCD color palettes, CGB-only)
        gb->video_dirty = true;
        log_warn("LCD color palettes write (addr = $%04X, value = $%02X)", addr,
                 value);
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select, CGB-only)
        log_warn("WRAM bank select (addr = $%04X, value = $%02X)", addr, value);
    } else if (addr == 0xFF7F) {
        // Tetris tries to write here. Probably a no-op.
    } else {
        log_warn("Unexpected I/O write (addr = $%04X, value = $%02X)", addr,
                 value);
    }
}

static void gb_write_mem_0000_8000(GameBoy *gb, u16 addr, u8 value)
{
    // 0000-7FFF (from cartridge)
    mapper_write(&gb->mapper, addr, value);
}

static void gb_write_mem_8000_A000(GameBoy *gb, u16 addr, u8 value)
{
    // 8000-9FFF (VRAM)
    gb->vram[addr - 0x8000] = value;
    gb->video_dirty = true;
}

static void gb_write_mem_A000_C000(GameBoy *gb, u16 addr, u8 value)
{
    // A000-BFFF (External RAM)
    mapper_write(&gb->mapper, addr, value);
}

static void gb_write_mem_C000_E000(GameBoy *gb, u16 addr, u8 value)
{
    // C000-DFFF (WRAM)
    gb->ram[addr - 0xC000] = value;
}

static void gb_write_mem_E000_10000(GameBoy *gb, u16 addr, u8 value)
{
    if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        gb->ram[addr - 0xE000] = value;
    } else if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        // TODO: should only be writable during HBlank or VBlank
        gb->oam[addr - 0xFE00] = value;
    } else if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        log_debug("Tried to write into unusable memory (addr = $%04X, $%02X)",
                  addr, value);
    } else if (addr <= 0xFF7F) {
        // FF00-FF7F I/O registers
        gb_write_io(gb, addr, value);
    } else if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        gb->hram[addr - 0xFF80] = value;
    } else {
        // FFFF (Interrupt Enable Register)
        gb->ie = value;
    }
}

static void gb_write_mem(GameBoy *gb, u16 addr, u8 value)
{
    static void (*const HANDLERS[])(GameBoy *, u16, u8) = {
        [0x0] = gb_write_mem_0000_8000,  [0x1] = gb_write_mem_0000_8000,
        [0x2] = gb_write_mem_0000_8000,  [0x3] = gb_write_mem_0000_8000,
        [0x4] = gb_write_mem_0000_8000,  [0x5] = gb_write_mem_0000_8000,
        [0x6] = gb_write_mem_0000_8000,  [0x7] = gb_write_mem_0000_8000,
        [0x8] = gb_write_mem_8000_A000,  [0x9] = gb_write_mem_8000_A000,
        [0xA] = gb_write_mem_A000_C000,  [0xB] = gb_write_mem_A000_C000,
        [0xC] = gb_write_mem_C000_E000,  [0xD] = gb_write_mem_C000_E000,
        [0xE] = gb_write_mem_E000_10000, [0xF] = gb_write_mem_E000_10000,
    };
    static_assert(ARRAY_LEN(HANDLERS) == 16);

    u8 nib = (addr >> 12) & 0xF;
    HANDLERS[nib](gb, addr, value);
}

static void gb_service_interrupts(GameBoy *gb, Memory mem)
{
    u8 int_mask = gb->if_ & gb->ie;

    // Disable HALT on an interrupt
    if (int_mask != 0 && gb->cpu.mode == CPU_MODE_HALTED)
        gb->cpu.mode = CPU_MODE_RUNNING;

    if (!gb->cpu.ime)
        return;

    // https://gbdev.io/pandocs/Interrupts.html
    for (size_t i = 0; i <= 4; ++i) {
        if (int_mask & (1 << i)) {
            log_debug("Servicing interrupt #%zu", i);
            gb->if_ &= ~(1 << i);
            cpu_interrupt(&gb->cpu, mem, 0x40 | (i << 3));
            break;
        }
    }
}

static u8 gb_read_mem_v(void *ptr, u16 addr)
{
    return gb_read_mem(ptr, addr);
}

static void gb_write_mem_v(void *ptr, u16 addr, u8 value)
{
    gb_write_mem(ptr, addr, value);
}

u64 gb_dispatch_cpu_instr(GameBoy *gb)
{
    static const MemoryVTable GB_MEMORY_VTABLE = {
        .read = gb_read_mem_v,
        .write = gb_write_mem_v,
    };

    Memory memory = (Memory){
        .ptr = gb,
        .vtable = &GB_MEMORY_VTABLE,
    };

    u64 mcycles_start = gb->cpu.mcycle_cnt;

    gb_update_joyp(gb);
    gb_service_interrupts(gb, memory);
    cpu_step(&gb->cpu, memory);

    u64 mcycles_end = gb->cpu.mcycle_cnt;
    return CPU_MCYCLE * (mcycles_end - mcycles_start);
}

static void gb_render_tile(GameBoy *gb, const u8 *tdata, u8 ti, size_t ty,
                           size_t tx)
{
    long tile_index_signed = (gb->lcdc & LCDC_BG_WIN_TILES) != 0 ? ti : (i8)ti;

    for (size_t tile_row_index = 0; tile_row_index < 8; ++tile_row_index) {
        u8 byte_1 = tdata[(tile_index_signed * 16) + (2 * tile_row_index)];
        u8 byte_2 = tdata[(tile_index_signed * 16) + (2 * tile_row_index) + 1];

        for (size_t tile_col_index = 0; tile_col_index < 8; ++tile_col_index) {
            u8 bit_lo = (byte_1 >> tile_col_index) & 1;
            u8 bit_hi = (byte_2 >> tile_col_index) & 1;
            u8 palette_index = bit_lo | (bit_hi << 1);

            u8 color = (gb->bgp >> (2 * palette_index)) & 0b11;

            size_t pixel_y = (8 * ty) + tile_row_index;
            size_t pixel_x = (8 * tx) + 7 - tile_col_index;

            assert(pixel_y < GB_BG_HEIGHT);
            assert(pixel_x < GB_BG_WIDTH);

            if (color != 0)
                gb->render_buf[pixel_y][pixel_x] = color;
        }
    }
}

static void gb_render_tiles(GameBoy *gb)
{
    static constexpr size_t TILES_HORIZONTAL = 32;
    static constexpr size_t TILES_VERTICAL = 32;

    size_t tdata_start = (gb->lcdc & LCDC_BG_WIN_TILES) != 0 ? 0 : 0x1000;
    size_t tmap_start = (gb->lcdc & LCDC_BG_TILE_MAP) != 0 ? 0x1C00 : 0x1800;

    const u8 *tdata = &gb->vram[tdata_start];
    const u8 *tmap = &gb->vram[tmap_start];

    for (size_t ty = 0; ty < TILES_VERTICAL; ++ty) {
        for (size_t tx = 0; tx < TILES_HORIZONTAL; ++tx) {
            u8 ti = tmap[(ty * TILES_HORIZONTAL) + tx];
            gb_render_tile(gb, tdata, ti, ty, tx);
        }
    }
}

typedef enum : u8 {
    OBJ_PRIORITY_LOW,
    OBJ_PRIORITY_HIGH,
} ObjPriority;

static void gb_render_obj(GameBoy *gb, const u8 *obj_data, ObjPriority priority)
{
    u8 attrs = obj_data[3];
    ObjPriority obj_priority = (attrs & OBJ_ATTRS_PRIORITY) == 0
                                   ? OBJ_PRIORITY_HIGH
                                   : OBJ_PRIORITY_LOW;

    if (obj_priority != priority)
        return;

    size_t y_pos = obj_data[0] - 16;
    size_t x_pos = obj_data[1] - 8;
    size_t tile_index = obj_data[2];

    bool flip_x = (attrs & OBJ_ATTRS_FLIP_X) != 0;
    bool flip_y = (attrs & OBJ_ATTRS_FLIP_Y) != 0;
    u8 obp = (attrs & OBJ_ATTRS_DMG_PALETTE) != 0 ? gb->obp1 : gb->obp0;

    for (size_t row = 0; row < 8; ++row) {
        // Objects always use the $8000 method
        u8 byte_1 = gb->vram[(16 * tile_index) + (2 * row)];
        u8 byte_2 = gb->vram[(16 * tile_index) + (2 * row) + 1];

        for (size_t col = 0; col < 8; ++col) {
            u8 lo = (byte_1 >> col) & 1;
            u8 hi = (byte_2 >> col) & 1;
            u8 pal_idx = lo | (hi << 1);
            u8 color = (obp >> (2 * pal_idx)) & 0b11;

            if (color != 0) {
                size_t py = gb->scy + y_pos + (flip_y ? 7 - row : row);
                size_t px = gb->scx + x_pos + (flip_x ? col : 7 - col);

                gb->render_buf[py % GB_BG_HEIGHT][px % GB_BG_WIDTH] = color;
            }
        }
    }
}

static void gb_render_objs(GameBoy *gb, ObjPriority priority)
{
    static constexpr size_t OBJ_COUNT = 40;

    for (size_t obj = 0; obj < OBJ_COUNT; ++obj) {
        const u8 *obj_data = &gb->oam[4 * obj];
        gb_render_obj(gb, obj_data, priority);
    }
}

static void gb_render(GameBoy *gb)
{
    memset(gb->render_buf, 0, GB_BG_HEIGHT * sizeof(*gb->render_buf));

    if ((gb->lcdc & LCDC_ENABLE) != 0) {
        if ((gb->lcdc & LCDC_OBJ_ENABLE) != 0)
            gb_render_objs(gb, OBJ_PRIORITY_LOW);

        gb_render_tiles(gb);

        if ((gb->lcdc & LCDC_OBJ_ENABLE) != 0)
            gb_render_objs(gb, OBJ_PRIORITY_HIGH);
    }
}

static void gb_ensure_rendered(GameBoy *gb)
{
    if (!gb->video_dirty)
        return;

    gb_render(gb);
    gb->video_dirty = false;
}

static u8 gb_scan_rendered(GameBoy *gb, size_t sy, size_t sx)
{
    assert(sy < GB_LCD_HEIGHT);
    assert(sx < GB_LCD_WIDTH);

    gb_ensure_rendered(gb);

    return gb->render_buf[(sy + gb->scy) % GB_BG_HEIGHT]
                         [(sx + gb->scx) % GB_BG_WIDTH];
}

static u8 calc_ppu_mode(u8 ly, u16 lx)
{
    if (ly >= GB_LCD_HEIGHT)
        return 1;

    if (lx < 80)
        return 2;

    if (lx < 252)
        return 3;

    return 0;
}

u64 gb_dispatch_pixel(GameBoy *gb)
{
    static constexpr u16 GB_DOTS = 456;
    static constexpr u16 GB_LINES = 154;
    static constexpr u16 GB_DOTS_DRAW_BEGIN = 80;
    static constexpr u8 GB_LY_VBLANK = 144;

    if (gb->ly < GB_LCD_HEIGHT && gb->lx >= GB_DOTS_DRAW_BEGIN) {
        size_t y = gb->ly;
        size_t x = gb->lx - GB_DOTS_DRAW_BEGIN;

        if (x < GB_LCD_WIDTH)
            gb->scanout_buf[y][x] = gb_scan_rendered(gb, y, x);
    }

    u8 ppu_mode_prev = calc_ppu_mode(gb->ly, gb->lx);

    ++gb->lx;
    assert(gb->lx <= GB_DOTS);

    if (gb->lx == GB_DOTS) {
        gb->lx = 0;
        gb->ly = (gb->ly + 1) % GB_LINES;

        bool lcy_eq_ly = gb->ly == gb->lcy;

        set_bits(&gb->stat, STAT_LCY_EQ_LY, lcy_eq_ly);

        // vblank interrupt
        if (gb->ly == GB_LY_VBLANK)
            gb->if_ |= INT_VBLANK;

        // stat lcy == ly interrupt
        if ((gb->stat & STAT_LYC_INT) != 0 && lcy_eq_ly)
            gb->if_ |= INT_LCD;
    }

    u8 ppu_mode = calc_ppu_mode(gb->ly, gb->lx);
    assert(ppu_mode < 4);
    gb->stat = (gb->stat & ~STAT_PPU_MODE) | ppu_mode;

    // TEST: need testing
    if (ppu_mode != ppu_mode_prev) {
        if (ppu_mode == 0 && (gb->stat & STAT_MODE0_INT) != 0)
            gb->if_ |= INT_LCD;

        if (ppu_mode == 1 && (gb->stat & STAT_MODE1_INT) != 0)
            gb->if_ |= INT_LCD;

        if (ppu_mode == 2 && (gb->stat & STAT_MODE2_INT) != 0)
            gb->if_ |= INT_LCD;
    }

    return 1;
}

u64 gb_dispatch_div(GameBoy *gb)
{
    if (gb->cpu.mode != CPU_MODE_STOPPED)
        ++gb->div;

    return 256;
}

// See
// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control
u64 gb_dispatch_tima(GameBoy *gb)
{
    if (gb->tac & 0b100)
        ++gb->tima;

    if (gb->tima == 0) {
        gb->tima = gb->tma;
        gb->if_ |= INT_TIMER;
    }

    u8 clock_select = gb->tac & 0b11;
    u64 tima_mcycles = clock_select == 0 ? 256 : 4 * clock_select;
    return CPU_MCYCLE * tima_mcycles;
}

// See https://gbdev.io/pandocs/OAM_DMA_Transfer.html
u64 gb_dispatch_dma_cp(GameBoy *gb)
{
    u8 lo = gb->dma_cur_addr & 0xFF;
    gb->oam[lo] = gb_read_mem(gb, gb->dma_cur_addr);

    ++gb->dma_cur_addr;

    if ((gb->dma_cur_addr & 0xFF) == OAM_SIZE)
        return SIZE_MAX; // done

    return 4;
}
