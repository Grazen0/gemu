#include "game_boy.h"
#include <stdint.h>
#include <stdlib.h>
#include "boot_rom.h"
#include "common/control.h"
#include "common/log.h"
#include "common/num.h"
#include "cpu.h"

const int GB_LCD_WIDTH = 160;
const int GB_LCD_HEIGHT = 144;
const int GB_BG_WIDTH = 256;
const int GB_BG_HEIGHT = 256;
const int GB_LCD_MAX_LY = 154;
const int GB_CPU_FREQ = 4194304;  // Hz
const double GB_CPU_FREQ_M = GB_CPU_FREQ / 4.0;

const uint8_t LCDC_ENABLE = 1 << 7;
const uint8_t LCDC_WIN_TILE_MAP = 1 << 6;
const uint8_t LCDC_WIN_ENABLE = 1 << 5;
const uint8_t LCDC_BGW_TILE_AREA = 1 << 4;
const uint8_t LCDC_BG_TILE_MAP = 1 << 3;
const uint8_t LCDC_OBJ_SIZE = 1 << 2;
const uint8_t LCDC_OBJ_ENABLE = 1 << 1;
const uint8_t LCDC_OBJ_BGW_ENABLE = 1 << 0;

GameBoy GameBoy_new(uint8_t* const rom, const size_t rom_len) {
    GameBoy gb = (GameBoy){
        .cpu = Cpu_new(),
        .rom = rom,
        .rom_len = rom_len,
        .rom_enable = true,
        .lcdc = 0,
        .lcds = 0,
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
        .iff = 0,
        .sb = 0,
        .sc = 0,
        .div = 0,
        .tima = 0,
        .tma = 0,
        .tac = 0,
    };

    return gb;
}

void GameBoy_destroy(GameBoy* const restrict gb) {
    free(gb->rom);
    gb->rom = NULL;
    gb->rom_len = 0;
}

uint8_t GameBoy_read_io(const GameBoy* const restrict gb, const uint16_t addr) {
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        // TODO: implement joypad stuff
        log_info("I/O joypad input read (0x%04X)", addr);
        return 0xF;
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        log_info("I/O serial transfer read (0x%04X)", addr);
        return gb->sb;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        log_info("I/O serial transfer read (0x%04X)", addr);
        return gb->sc;
    } else if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)
        switch (addr) {
            case 0xFF04:
                return gb->div;
                break;
            case 0xFF05:
                return gb->tima;
                break;
            case 0xFF06:
                return gb->tma;
                break;
            case 0xFF07:
                return gb->tac;
                break;
            default:
                bail("Unexpected I/O timer and divider read (0x%04X)", addr);
        }
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        return gb->iff;
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        bail("I/O audio read (0x%04X)", addr);
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        bail("I/O wave pattern read (0x%04X)", addr);
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        switch (addr) {
            // LCD control
            case 0xFF40:
                return gb->lcdc;

            // LCD status registers
            case 0xFF44:
                return gb->ly;
            case 0xFF45:
                return gb->lcy;
            case 0xFF41:
                return gb->lcds;

            // Scrolling
            case 0xFF42:
                return gb->scy;
            case 0xFF43:
                return gb->scx;
            case 0xFF4A:
                return gb->wy;
            case 0xFF4B:
                return gb->wx;

            // Palettes
            case 0xFF47:
                return gb->bgp;
            case 0xFF48:
                return gb->obp0;
            case 0xFF49:
                return gb->obp1;

            default:
                bail("Unexpected I/O LCD read (addr = 0x%04X)", addr);
        }
    } else if (addr == 0xFF4F) {
        // FF4F
        bail("I/O VRAM bank select read (0x%04X)", addr);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        return 0xFF;
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA)
        bail("I/O VRAM DMA read (0x%04X)", addr);
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (palettes)
        bail("I/O palettes read (0x%04X)", addr);
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select)
        bail("I/O WRAM bank select read (0x%04X)", addr);
    } else {
        bail("Unexpected I/O read (addr = 0x%04X)", addr);
        return 0xFF;
    }
}

uint8_t GameBoy_read_mem(const void* const restrict ctx, const uint16_t addr) {
    const GameBoy* const restrict gb = ctx;

    if (addr <= 0x7FFF) {
        if (gb->rom_enable && addr <= 0xFF) {
            // 0000-0100 (Boot ROM)
            return BOOT_ROM[addr];
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
        bail("TODO: GameBoy_read_mem (addr = %04X)", addr);
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
        bail("TODO: GameBoy_read_mem (addr = 0x%04X)", addr);
    }

    if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        bail("Tried to read unusable memory (addr = 0x%04X)", addr);
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
    log_info("TODO: GameBoy_read_mem IE (addr = 0x%04X)", addr);
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
        log_info("I/O joypad input write (0x%04X, 0x%02X)", addr, value);
        gb->joyp = value & 0xF0;
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        log_info("I/O serial transfer write (0x%04X, 0x%02X)", addr, value);
        gb->sb = value;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        log_info("I/O serial transfer write (0x%04X, 0x%02X)", addr, value);
        gb->sc = value;
    } else if (addr >= 0xFF04 && addr <= 0xFF07) {
        // FF04-FF07 (timer and divider)
        switch (addr) {
            case 0xFF04:
                gb->div = 0;
                break;
            case 0xFF05:
                gb->tima = value;
                break;
            case 0xFF06:
                gb->tma = value;
                break;
            case 0xFF07:
                gb->tac = value;
                break;
            default:
                bail("Unexpected I/O timer and divider write (0x%04X, 0x%02X)", addr, value);
        }
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        gb->iff = value;
        log_info("I/O IF write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        log_info("I/O audio write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        bail("I/O wave pattern write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF46) {
        // FF46 (OAM DMA source address and start)
        log_info("OAM DMA source/start write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        log_info("I/O LCD write (0x%04X, 0x%02X)", addr, value);

        switch (addr) {
            // LCD control
            case 0xFF40:
                gb->lcdc = value;
                break;

            // LCD status registers
            case 0xFF45:
                gb->lcy = value;
                break;
            case 0xFF41:
                gb->lcds = value;
                break;

            // Scrolling
            case 0xFF42:
                gb->scy = value;
                break;
            case 0xFF43:
                gb->scx = value;
                break;
            case 0xFF4A:
                gb->wy = value;
                break;
            case 0xFF4B:
                gb->wx = value;
                break;

            // Palettes
            case 0xFF47:
                gb->bgp = value;
                break;
            case 0xFF48:
                gb->obp0 = value;
                break;
            case 0xFF49:
                gb->obp1 = value;
                break;

            default:
                bail("Unexpected I/O LCD write (addr = 0x%04X, value = 0x%02X)", addr, value);
        }
    } else if (addr == 0xFF4F) {
        // FF4F
        bail("I/O VRAM bank select write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        if (value != 0) {
            gb->rom_enable = false;
        }
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA)
        bail("I/O VRAM DMA write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (palettes)
        bail("I/O palettes write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select)
        bail("I/O WRAM bank select write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF7F) {
        // Not sure what to do here
        log_info("Unexpected I/O LCD write (addr = 0x%04X, value = 0x%02X)", addr, value);
    } else {
        bail("Unexpected I/O write (addr = 0x%04X, value = 0x%02X)", addr, value);
    }
}

void GameBoy_write_mem(void* const restrict ctx, const uint16_t addr, const uint8_t value) {
    GameBoy* const restrict gb = ctx;

    log_info("write mem (addr = 0x%04X, value = 0x%02X)", addr, value);

    if (addr <= 0x7FFF) {
        // 0000-7FFF (ROM bank)
        log_info("TODO: GameBoy_write_mem ROM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0x9FFF) {
        // 8000-9FFF (VRAM)
        gb->vram[addr - 0x8000] = value;
    } else if (addr <= 0xBFFF) {
        // A000-BFFF (External RAM)
        bail("TODO: GameBoy_write_mem ERAM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xDFFF) {
        // C000-DFFF (WRAM)
        gb->ram[addr - 0xC000] = value;
    } else if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        gb->ram[addr - 0xE000] = value;
    } else if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        log_info("TODO: GameBoy_write_mem OAM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)

        log_info("Tried to write into unusable memory (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xFF7F) {
        // FF00-FF7F I/O registers
        GameBoy_write_io(gb, addr, value);
    } else if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        gb->hram[addr - 0xFF80] = value;
    } else {
        // FFFF (Interrupt Enable Register)
        log_info("GameBoy_write_mem IE (addr = 0x%04X, 0x%02X)", addr, value);
        gb->ie = value;
    }
}
