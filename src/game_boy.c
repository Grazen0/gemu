#include "game_boy.h"
#include "control.h"
#include "cpu.h"
#include "data.h"
#include "log.h"
#include "num.h"
#include "stdinc.h"
#include "string.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void GameBoy_write_joyp(GameBoy *const self, const u8 value)
{
    self->joyp = value | 0x0F;

    if ((self->joyp & Joypad_DPadSelect) == 0) {
        if (self->joypad.right)
            self->joyp &= ~Joypad_RightA;

        if (self->joypad.left)
            self->joyp &= ~Joypad_LeftB;

        if (self->joypad.up)
            self->joyp &= ~Joypad_UpSelect;

        if (self->joypad.down)
            self->joyp &= ~Joypad_DownStart;
    }

    if ((self->joyp & Joypad_ButtonsSelect) == 0) {
        if (self->joypad.a)
            self->joyp &= ~Joypad_RightA;

        if (self->joypad.b)
            self->joyp &= ~Joypad_LeftB;

        if (self->joypad.select)
            self->joyp &= ~Joypad_UpSelect;

        if (self->joypad.start)
            self->joyp &= ~Joypad_DownStart;
    }
}

static void verify_rom_checksum(const u8 *const rom)
{
    u8 checksum = 0;
    for (u16 addr = 0x0134; addr <= 0x014C; ++addr)
        checksum = checksum - rom[addr] - 1;

    const u8 checksum_lo = checksum & 0x0F;

    BAIL_IF(
        checksum_lo != rom[RomHeader_HeaderChecksum],
        "Lower 8 bits of ROM checksum do not match expected valuein header (expected $%02X, was $%02X)",
        rom[RomHeader_HeaderChecksum], checksum_lo);
}

static void GameBoy_simulate_boot(GameBoy *const self)
{
    verify_rom_checksum(self->rom);

    self->cpu.a = 0x01;
    self->cpu.b = 0x00;
    self->cpu.c = 0x13;
    self->cpu.d = 0x00;
    self->cpu.e = 0xD8;
    self->cpu.h = 0x01;
    self->cpu.l = 0x4D;
    self->cpu.sp = 0xFFFE;
    self->cpu.pc = 0x0100;

    self->boot_rom_enable = false;
}

static void GameBoy_reset(GameBoy *const self)
{
    self->cpu.pc = 0;
    self->boot_rom_enable = true;
}

static void GameBoy_validate_rom(const GameBoy *const self)
{
    BAIL_IF(self->rom[RomHeader_CartridgeType] != 0x00,
            "Unsupported cartridge type (ctype: $%02X)",
            self->rom[RomHeader_CartridgeType]);

    BAIL_IF(
        !CartridgeType_has_ram(self->rom[RomHeader_CartridgeType]) &&
            self->rom[RomHeader_RamSize] != 0,
        "Cartridge type does not have RAM, but header indicates otherwise (ctype: $%02, RAM size: $%02X)",
        self->rom[RomHeader_CartridgeType], self->rom[RomHeader_RamSize]);

    BAIL_IF(
        self->rom_len != 0x8000 * ((size_t)1 << self->rom[RomHeader_RomSize]),
        "Actual ROM size does not match header-specified size. (specified: $%02X, was: $%02X)",
        self->rom[RomHeader_RomSize], self->rom_len);
}

GameBoy GameBoy_new(u8 *const boot_rom)
{
    return (GameBoy){
        .cpu = Cpu_new(),
        .boot_rom = boot_rom,
        .rom = nullptr,
        .rom_len = 0,
        .boot_rom_enable = true,
        .lcdc = 0,
        .stat = 0,
        .ly = 0,
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
    };
}

void GameBoy_destroy(GameBoy *const self)
{
    free(self->rom);
    free(self->boot_rom);

    self->rom = nullptr;
    self->rom_len = 0;
}

void GameBoy_log_cartridge_info(const GameBoy *const self)
{
    log_info("Cartridge type: $%02X", self->rom[RomHeader_CartridgeType]);
    log_info("RAM size: $%02X", self->rom[RomHeader_RamSize]);
    log_info("ROM size: $%02X", self->rom[RomHeader_RomSize]);

    char game_title[17];
    strlcpy(game_title, (char *)&self->rom[RomHeader_Title],
            sizeof(game_title));
    log_info("Game title: %s", game_title);
}

void GameBoy_load_rom(GameBoy * self, u8 *rom, size_t rom_len)
{
    if (rom == nullptr)
        BAIL("Cannot load null ROM");

    free(self->rom);

    self->rom = rom;
    self->rom_len = rom_len;

    GameBoy_reset(self);
    GameBoy_validate_rom(self);

    if (self->boot_rom == nullptr)
        GameBoy_simulate_boot(self);
}

// NOLINTNEXTLINE
u8 GameBoy_read_io(const GameBoy *const self, const u16 addr)
{
    if (addr == 0xFF00) // FF00 (joypad input)
        return self->joyp;

    // TODO: implement serial transfer
    if (addr == 0xFF01) // FF01 (serial transfer data)
        return 0xFF;

    if (addr == 0xFF02) // FF02 (serial transfer control)
        return self->sc;

    if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)

        // clang-format off
        switch (addr) {
            case 0xFF04: return self->div;
            case 0xFF05: return self->tima;
            case 0xFF06: return self->tma;
            case 0xFF07: return self->tac;
            default: BAIL("Unexpected I/O timer and divider read ($%04X)", addr);
        }
        // clang-format on
    }

    if (addr == 0xFF0F) // FF0F (interrupts)
        return self->if_;

    if (addr >= 0xFF10 && addr <= 0xFF26) // FF10-FF26 (audio)
        BAIL("I/O audio read ($%04X)", addr);

    if (addr >= 0xFF30 && addr <= 0xFF3F) // FF30-FF3F (wave pattern)
        BAIL("I/O wave pattern read ($%04X)", addr);

    if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        // clang-format off
        switch (addr) {
            case 0xFF40: return self->lcdc;
            case 0xFF44: return self->ly;
            case 0xFF45: return self->lcy;
            case 0xFF41: return self->stat;
            case 0xFF42: return self->scy;
            case 0xFF43: return self->scx;
            case 0xFF4A: return self->wy;
            case 0xFF4B: return self->wx;
            case 0xFF47: return self->bgp;
            case 0xFF48: return self->obp0;
            case 0xFF49: return self->obp1;
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

u8 GameBoy_read_mem(const void *const ctx, const u16 addr)
{
    const GameBoy *const self = ctx;

    if (addr <= 0x7FFF) {
        if (self->boot_rom_enable && addr <= 0x100) {
            // 0000-0100 (Boot ROM)
            if (self->boot_rom == nullptr)
                BAIL("Tried to read non-existing boot ROM");

            return self->boot_rom[addr];
        }

        if (self->rom == nullptr)
            BAIL("Tried to read non-existing ROM");

        // 0000-7FFF (ROM bank)
        return self->rom[addr];
    }

    if (addr <= 0x9FFF) // 8000-9FFF (VRAM)
        return self->vram[addr - 0x8000];

    if (addr <= 0xBFFF) // A000-BFFF (External RAM)
        BAIL("TODO: GameBoy_read_mem (addr = $%04X)", addr);

    if (addr <= 0xDFFF) // C000-DFFF (WRAM)
        return self->ram[addr - 0xC000];

    if (addr <= 0xFDFF) // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        return self->ram[addr - 0xE000];

    if (addr <= 0xFE9F) // FE00-FE9F (OAM)
        return self->oam[addr - 0xFE00];

    if (addr <= 0xFEFF) // FEA0-FEFF (Not usable)
        BAIL("Tried to read unusable memory (addr = $%04X)", addr);

    if (addr <= 0xFF7F) // FF00-FF7F (I/O registers)
        return GameBoy_read_io(self, addr);

    if (addr <= 0xFFFE) // FF80-FFFE (High RAM)
        return self->hram[addr - 0xFF80];

    // FFFF (Interrupt Enable Register)
    return self->ie;
}

u16 GameBoy_read_mem_u16(GameBoy *const self, u16 addr)
{
    const u8 lo = GameBoy_read_mem(self, addr);
    const u8 hi = GameBoy_read_mem(self, addr + 1);
    return concat_u16(hi, lo);
}

void GameBoy_write_io(GameBoy *const self, const u16 addr,
                      const u8 value)
{
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        GameBoy_write_joyp(self, value);
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        self->sb = value;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        // TODO: implement properly
        self->sc = value;
    } else if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)
        // clang-format off
        switch (addr) {
            case 0xFF04: self->div = 0; break;
            case 0xFF05: self->tima = value; break;
            case 0xFF06: self->tma = value; break;
            case 0xFF07: self->tac = value; break;
            default: BAIL("Unexpected I/O timer and divider write ($%04X, $%02X)", addr, value);
        }
        // clang-format on
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        self->if_ = value;
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        // TODO: I/O audio write
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        // TODO: I/O wave pattern write
    } else if (addr == 0xFF46) {
        // FF46 (OAM DMA source address and start)
        const u16 src = (u16)value << 8;

        // TODO: implement proper timing
        for (size_t i = 0; i < 0xA0; ++i) {
            self->oam[i] = GameBoy_read_mem(self, src + i);
        }
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        // clang-format off
        switch (addr) {
            case 0xFF40: self->lcdc = value; break;
            case 0xFF45: self->lcy = value; break;
            case 0xFF41:
                // Modifies only bits 3-7
                self->stat = (self->stat & 0b111) | (value & ~0b111);
                break;
            case 0xFF42: self->scy = value; break;
            case 0xFF43: self->scx = value; break;
            case 0xFF4A: self->wy = value; break;
            case 0xFF4B: self->wx = value; break;
            case 0xFF47: self->bgp = value; break;
            case 0xFF48: self->obp0 = value; break;
            case 0xFF49: self->obp1 = value; break;
            default: BAIL("Unexpected I/O LCD write (addr = $%04X, value = $%02X)", addr, value);
        }
        // clang-format on
    } else if (addr == 0xFF4F) {
        // FF4F
        BAIL("I/O VRAM bank select write ($%04X, $%02X)", addr, value);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        if (value != 0)
            self->boot_rom_enable = false;
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA, CGB-only)
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (LCD color palettes, CGB-only)
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select, CGB-only)
    } else if (addr == 0xFF7F) {
        // Tetris tries to write here. Probably a no-op.
    } else {
        BAIL("Unexpected I/O write (addr = $%04X, value = $%02X)", addr, value);
    }
}

void GameBoy_write_mem(void *const ctx, const u16 addr, const u8 value)
{
    GameBoy *const self = ctx;

    log_trace("write mem (addr = $%04X, value = $%02X)", addr, value);

    if (addr <= 0x7FFF) {
        // 0000-7FFF (ROM bank)
        log_debug("TODO: GameBoy_write_mem ROM (addr = $%04X, $%02X)", addr,
                  value);
    } else if (addr <= 0x9FFF) {
        // 8000-9FFF (VRAM)
        self->vram[addr - 0x8000] = value;
    } else if (addr <= 0xBFFF) {
        // A000-BFFF (External RAM)
        BAIL("TODO: GameBoy_write_mem ERAM (addr = $%04X, $%02X)", addr, value);
    } else if (addr <= 0xDFFF) {
        // C000-DFFF (WRAM)
        self->ram[addr - 0xC000] = value;
    } else if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        self->ram[addr - 0xE000] = value;
    } else if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        // TODO: should only be writable during HBlank or VBlank
        self->oam[addr - 0xFE00] = value;
    } else if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        log_debug("Tried to write into unusable memory (addr = $%04X, $%02X)",
                  addr, value);
    } else if (addr <= 0xFF7F) {
        // FF00-FF7F I/O registers
        GameBoy_write_io(self, addr, value);
    } else if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        self->hram[addr - 0xFF80] = value;
    } else {
        // FFFF (Interrupt Enable Register)
        self->ie = value;
    }
}

void GameBoy_service_interrupts(GameBoy *const self,
                                Memory *const mem)
{
    const u8 int_mask = self->if_ & self->ie;

    // Disable HALT on an interrupt
    if (int_mask != 0 && self->cpu.mode == CpuMode_Halted)
        self->cpu.mode = CpuMode_Running;

    if (!self->cpu.ime)
        return;

    for (size_t i = 0; i <= 4; ++i) {
        if (int_mask & (1 << i)) {
            log_debug("Servicing interrupt #%zu", i);
            self->if_ &= ~(1 << i);
            Cpu_interrupt(&self->cpu, mem, 0x40 | (i << 3));
            break;
        }
    }
}
