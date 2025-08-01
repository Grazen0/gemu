#include "game_boy.h"
#include "control.h"
#include "cpu.h"
#include "data.h"
#include "log.h"
#include "num.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void GameBoy_write_joyp(GameBoy* const restrict gb, const uint8_t value) {
    gb->joyp = value | 0x0F;

    if ((gb->joyp & Joypad_DPadSelect) == 0) {
        if (gb->joypad.right) {
            gb->joyp &= ~Joypad_RightA;
        }
        if (gb->joypad.left)
            gb->joyp &= ~Joypad_LeftB;

        if (gb->joypad.up)
            gb->joyp &= ~Joypad_UpSelect;

        if (gb->joypad.down)
            gb->joyp &= ~Joypad_DownStart;
    }

    if ((gb->joyp & Joypad_ButtonsSelect) == 0) {
        if (gb->joypad.a)
            gb->joyp &= ~Joypad_RightA;

        if (gb->joypad.b)
            gb->joyp &= ~Joypad_LeftB;

        if (gb->joypad.select)
            gb->joyp &= ~Joypad_UpSelect;

        if (gb->joypad.start)
            gb->joyp &= ~Joypad_DownStart;
    }
}

static void verify_rom_checksum(const uint8_t* const rom) {
    uint8_t checksum = 0;
    for (uint16_t addr = 0x0134; addr <= 0x014C; ++addr) {
        checksum = checksum - rom[addr] - 1;
    }

    const uint8_t checksum_lo = checksum & 0x0F;

    if (checksum_lo != rom[RomHeader_HeaderChecksum]) {
        log_error(
            LogCategory_Keep,
            "Lower 8 bits of ROM checksum do not match expected value in header (expected "
            "$%02X, was $%02X)",
            rom[RomHeader_HeaderChecksum],
            checksum_lo
        );
        exit(1);
    }
}

static void GameBoy_simulate_boot(GameBoy* const gb) {
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

GameBoy GameBoy_new(uint8_t* const boot_rom, uint8_t* const rom, const size_t rom_len) {
    GameBoy gb = (GameBoy){
        .cpu = Cpu_new(),
        .boot_rom = boot_rom,
        .rom = rom,
        .rom_len = rom_len,
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

    if (boot_rom == nullptr)
        GameBoy_simulate_boot(&gb);

    return gb;
}

void GameBoy_destroy(GameBoy* const restrict gb) {
    free(gb->rom);
    if (gb->boot_rom != nullptr) {
        free(gb->boot_rom);
    }
    gb->rom = nullptr;
    gb->rom_len = 0;
}

uint8_t GameBoy_read_io(const GameBoy* const restrict gb, const uint16_t addr) {
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        return gb->joyp;
    }

    if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        // TODO: implement serial transfer
        return 0xFF;
    }

    if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        return gb->sc;
    }

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

    if (addr == 0xFF0F) {
        // FF0F (interrupts)
        return gb->if_;
    }

    if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        BAIL("I/O audio read ($%04X)", addr);
    }

    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        BAIL("I/O wave pattern read ($%04X)", addr);
    }

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

    if (addr == 0xFF4D) {
        // FF4D (CGB registers, CGB-only)
        return 0xFF;
    }

    if (addr == 0xFF4F) {
        // FF4F (VRAM bank select, CGB-only)
    }

    if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        return 0xFF;
    }

    if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA, CGB-only)
        return 0xFF;
    }

    if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (LCD color palettes, CGB-only)
        return 0xFF;
    }

    if (addr == 0xFF70) {
        // FF70 (WRAM bank select, CGB-only)
        return 0xFF;
    }

    BAIL("Unexpected I/O read (addr = $%04X)", addr);
}

uint8_t GameBoy_read_mem(const void* const restrict ctx, const uint16_t addr) {
    const GameBoy* const gb = ctx;

    if (addr <= 0x7FFF) {
        if (gb->boot_rom_enable && addr <= 0x100) {
            // 0000-0100 (Boot ROM)
            if (gb->boot_rom == nullptr)
                BAIL("Tried to read non-existing boot ROM");

            return gb->boot_rom[addr];
        }

        // 0100-7FFF (ROM bank)
        return gb->rom[addr];
    }

    if (addr <= 0x9FFF) {
        // 8000-9FFF (VRAM)
        return gb->vram[addr - 0x8000];
    }

    if (addr <= 0xBFFF) {
        // A000-BFFF (External RAM)
        BAIL("TODO: GameBoy_read_mem (addr = $%04X)", addr);
    }

    if (addr <= 0xDFFF) {
        // C000-DFFF (WRAM)
        return gb->ram[addr - 0xC000];
    }

    if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        return gb->ram[addr - 0xE000];
    }

    if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        return gb->oam[addr - 0xFE00];
    }

    if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        BAIL("Tried to read unusable memory (addr = $%04X)", addr);
    }

    if (addr <= 0xFF7F) {
        // FF00-FF7F (I/O registers)
        return GameBoy_read_io(gb, addr);
    }

    if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        return gb->hram[addr - 0xFF80];
    }

    // FFFF (Interrupt Enable Register)
    return gb->ie;
}

uint16_t GameBoy_read_mem_u16(GameBoy* const restrict gb, uint16_t addr) {
    const uint8_t lo = GameBoy_read_mem(gb, addr);
    const uint8_t hi = GameBoy_read_mem(gb, addr + 1);
    return concat_u16(hi, lo);
}

void GameBoy_write_io(GameBoy* const restrict gb, const uint16_t addr, const uint8_t value) {
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        GameBoy_write_joyp(gb, value);
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
        const uint16_t src = (uint16_t)value << 8;

        // TODO: implement proper timing
        for (size_t i = 0; i < 0xA0; ++i) {
            gb->oam[i] = GameBoy_read_mem(gb, src + i);
        }
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
            case 0xFF4A: gb->wy = value; break;
            case 0xFF4B: gb->wx = value; break;
            case 0xFF47: gb->bgp = value; break;
            case 0xFF48: gb->obp0 = value; break;
            case 0xFF49: gb->obp1 = value; break;
            default: BAIL("Unexpected I/O LCD write (addr = $%04X, value = $%02X)", addr, value);
        }
        // clang-format on
    } else if (addr == 0xFF4F) {
        // FF4F
        BAIL("I/O VRAM bank select write ($%04X, $%02X)", addr, value);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        if (value != 0)
            gb->boot_rom_enable = false;
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

void GameBoy_write_mem(void* const restrict ctx, const uint16_t addr, const uint8_t value) {
    GameBoy* const restrict gb = ctx;

    log_info(LogCategory_Memory, "write mem (addr = $%04X, value = $%02X)", addr, value);

    if (addr <= 0x7FFF) {
        // 0000-7FFF (ROM bank)
        log_info(
            LogCategory_Memory, "TODO: GameBoy_write_mem ROM (addr = $%04X, $%02X)", addr, value
        );
    } else if (addr <= 0x9FFF) {
        // 8000-9FFF (VRAM)
        gb->vram[addr - 0x8000] = value;
    } else if (addr <= 0xBFFF) {
        // A000-BFFF (External RAM)
        BAIL("TODO: GameBoy_write_mem ERAM (addr = $%04X, $%02X)", addr, value);
    } else if (addr <= 0xDFFF) {
        // C000-DFFF (WRAM)
        gb->ram[addr - 0xC000] = value;
    } else if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        gb->ram[addr - 0xE000] = value;
    } else if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        // TODO: should only be writable during HBlank or VBlank
        gb->oam[addr - 0xFE00] = value;
    } else if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        log_warn(
            LogCategory_Memory,
            "Tried to write into unusable memory (addr = $%04X, $%02X)",
            addr,
            value
        );
    } else if (addr <= 0xFF7F) {
        // FF00-FF7F I/O registers
        GameBoy_write_io(gb, addr, value);
    } else if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        gb->hram[addr - 0xFF80] = value;
    } else {
        // FFFF (Interrupt Enable Register)
        gb->ie = value;
    }
}

void GameBoy_service_interrupts(GameBoy* const restrict gb, Memory* const restrict mem) {
    const uint8_t int_mask = gb->if_ & gb->ie;

    // Disable HALT on an interrupt
    if (int_mask != 0 && gb->cpu.mode == CpuMode_Halted) {
        gb->cpu.mode = CpuMode_Running;
    }

    if (!gb->cpu.ime) {
        return;
    }

    for (uint8_t i = 0; i <= 4; ++i) {
        if (int_mask & (1 << i)) {
            log_warn(LogCategory_Interrupt, "Servicing interrupt #%i", i);
            gb->if_ &= ~(1 << i);
            Cpu_interrupt(&gb->cpu, mem, 0x40 | (i << 3));
            break;
        }
    }
}
