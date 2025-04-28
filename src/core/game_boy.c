#include "game_boy.h"
#include <stdint.h>
#include <stdlib.h>
#include "../lib/log.h"
#include "../lib/num.h"
#include "SDL3/SDL_log.h"
#include "boot_rom.h"
#include "cpu.h"

GameBoy GameBoy_new(uint8_t* const rom, const size_t rom_len) {
    return (GameBoy){
        .cpu = Cpu_new(),
        .cycle_count = 0,
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
        SDL_Log("I/O joypad input read (0x%04X)", addr);
        return 0xF;
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        SDL_Log("I/O serial transfer read (0x%04X)", addr);
        return gb->sb;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        SDL_Log("I/O serial transfer read (0x%04X)", addr);
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
                BAIL("Unexpected I/O timer and divider read (0x%04X)", addr);
        }
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        return gb->iff;
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        BAIL("I/O audio read (0x%04X)", addr);
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        BAIL("I/O wave pattern read (0x%04X)", addr);
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
                BAIL("Unexpected I/O LCD read (addr = 0x%04X)", addr);
        }
    } else if (addr == 0xFF4F) {
        // FF4F
        BAIL("I/O VRAM bank select read (0x%04X)", addr);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        return 0xFF;
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA)
        BAIL("I/O VRAM DMA read (0x%04X)", addr);
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (palettes)
        BAIL("I/O palettes read (0x%04X)", addr);
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select)
        BAIL("I/O WRAM bank select read (0x%04X)", addr);
    } else {
        BAIL("Unexpected I/O read (addr = 0x%04X)", addr);
        return 0xFF;
    }
}

uint8_t GameBoy_read_mem(GameBoy* const restrict gb, const uint16_t addr) {
    gb->cycle_count++;

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
        BAIL("TODO: GameBoy_read_mem (addr = %04X)", addr);
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
        BAIL("TODO: GameBoy_read_mem (addr = 0x%04X)", addr);
    }

    if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)
        BAIL("Tried to read unusable memory (addr = 0x%04X)", addr);
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
    SDL_Log("TODO: GameBoy_read_mem IE (addr = 0x%04X)", addr);
    return gb->ie;
}

uint16_t GameBoy_read_mem_u16(GameBoy* const restrict gb, uint16_t addr) {
    const uint8_t lo = GameBoy_read_mem(gb, addr);
    const uint8_t hi = GameBoy_read_mem(gb, addr + 1);
    return CONCAT_U16(hi, lo);
}

void GameBoy_write_io(GameBoy* const restrict gb, const uint16_t addr, const uint8_t value) {
    if (addr == 0xFF00) {
        // FF00 (joypad input)
        SDL_Log("I/O joypad input write (0x%04X, 0x%02X)", addr, value);
        gb->joyp = value & 0xF0;
    } else if (addr == 0xFF01) {
        // FF01 (serial transfer data)
        SDL_Log("I/O serial transfer write (0x%04X, 0x%02X)", addr, value);
        gb->sb = value;
    } else if (addr == 0xFF02) {
        // FF02 (serial transfer control)
        SDL_Log("I/O serial transfer write (0x%04X, 0x%02X)", addr, value);
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
                BAIL("Unexpected I/O timer and divider write (0x%04X, 0x%02X)", addr, value);
        }
    } else if (addr == 0xFF0F) {
        // FF0F (interrupts)
        gb->iff = value;
        SDL_Log("I/O IF write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF10 && addr <= 0xFF26) {
        // FF10-FF26 (audio)
        SDL_Log("I/O audio write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // FF30-FF3F (wave pattern)
        BAIL("I/O wave pattern write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF46) {
        // FF46 (OAM DMA source address and start)
        SDL_Log("OAM DMA source/start write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // FF40-FF4B (LCD)
        SDL_Log("I/O LCD write (0x%04X, 0x%02X)", addr, value);

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
                BAIL("Unexpected I/O LCD write (addr = 0x%04X, value = 0x%02X)", addr, value);
        }
    } else if (addr == 0xFF4F) {
        // FF4F
        BAIL("I/O VRAM bank select write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF50) {
        // FF50 (boot ROM disable)
        if (value != 0) {
            gb->rom_enable = false;
        }
    } else if (addr >= 0xFF51 && addr <= 0xFF55) {
        // FF51-FF55 (VRAM DMA)
        BAIL("I/O VRAM DMA write (0x%04X, 0x%02X)", addr, value);
    } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
        // FF68-FF6B (palettes)
        BAIL("I/O palettes write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF70) {
        // FF70 (WRAM bank select)
        BAIL("I/O WRAM bank select write (0x%04X, 0x%02X)", addr, value);
    } else if (addr == 0xFF7F) {
        // Not sure what to do here
        SDL_Log("Unexpected I/O LCD write (addr = 0x%04X, value = 0x%02X)", addr, value);
    } else {
        BAIL("Unexpected I/O write (addr = 0x%04X, value = 0x%02X)", addr, value);
    }
}

void GameBoy_write_mem(GameBoy* const restrict gb, const uint16_t addr, const uint8_t value) {
    SDL_Log("write mem (addr = 0x%04X, value = 0x%02X)", addr, value);
    gb->cycle_count++;

    if (addr <= 0x7FFF) {
        // 0000-7FFF (ROM bank)
        SDL_Log("TODO: GameBoy_write_mem ROM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0x9FFF) {
        // 8000-9FFF (VRAM)
        gb->vram[addr - 0x8000] = value;
    } else if (addr <= 0xBFFF) {
        // A000-BFFF (External RAM)
        BAIL("TODO: GameBoy_write_mem ERAM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xDFFF) {
        // C000-DFFF (WRAM)
        gb->ram[addr - 0xC000] = value;
    } else if (addr <= 0xFDFF) {
        // E000-FDFF (Echo RAM, mirror of C000-DDFF)
        gb->ram[addr - 0xE000] = value;
    } else if (addr <= 0xFE9F) {
        // FE00-FE9F (OAM)
        SDL_Log("TODO: GameBoy_write_mem OAM (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xFEFF) {
        // FEA0-FEFF (Not usable)

        SDL_Log("Tried to write into unusable memory (addr = 0x%04X, 0x%02X)", addr, value);
    } else if (addr <= 0xFF7F) {
        // FF00-FF7F I/O registers
        GameBoy_write_io(gb, addr, value);
    } else if (addr <= 0xFFFE) {
        // FF80-FFFE (High RAM)
        gb->hram[addr - 0xFF80] = value;
    } else {
        // FFFF (Interrupt Enable Register)
        SDL_Log("GameBoy_write_mem IE (addr = 0x%04X, 0x%02X)", addr, value);
        gb->ie = value;
    }
}

void GameBoy_write_mem_u16(GameBoy* const restrict gb, const uint16_t addr, uint16_t value) {
    GameBoy_write_mem(gb, addr, LO(value));
    GameBoy_write_mem(gb, addr + 1, HI(value));
}

uint8_t GameBoy_read_pc(GameBoy* const restrict gb) {
    const uint8_t value = GameBoy_read_mem(gb, gb->cpu.pc);
    gb->cpu.pc++;
    return value;
}

uint16_t GameBoy_read_pc_u16(GameBoy* const restrict gb) {
    const uint8_t lo = GameBoy_read_pc(gb);
    const uint8_t hi = GameBoy_read_pc(gb);
    return CONCAT_U16(hi, lo);
}

void GameBoy_stack_push_u16(GameBoy* const restrict gb, const uint16_t value) {
    GameBoy_write_mem(gb, gb->cpu.sp - 1, HI(value));
    GameBoy_write_mem(gb, gb->cpu.sp - 2, LO(value));
    gb->cpu.sp -= 2;
    gb->cycle_count++;
}

uint16_t GameBoy_stack_pop_u16(GameBoy* const restrict gb) {
    const uint8_t lo = GameBoy_read_mem(gb, gb->cpu.sp);
    const uint8_t hi = GameBoy_read_mem(gb, gb->cpu.sp + 1);
    gb->cpu.sp += 2;
    return CONCAT_U16(hi, lo);
}

uint8_t GameBoy_read_r(GameBoy* const restrict gb, const CpuTableR r) {
    switch (r) {
        case CPU_R_B:
            return gb->cpu.b;
        case CPU_R_C:
            return gb->cpu.c;
        case CPU_R_D:
            return gb->cpu.d;
        case CPU_R_E:
            return gb->cpu.e;
        case CPU_R_H:
            return gb->cpu.h;
        case CPU_R_L:
            return gb->cpu.l;
        case CPU_R_HL:
            return GameBoy_read_mem(gb, Cpu_read_rp(&gb->cpu, CPU_RP_HL));
        case CPU_R_A:
            return gb->cpu.a;
        default:
            BAIL("invalid cpu r: %i", r);
    }
}

void GameBoy_write_r(GameBoy* const restrict gb, const CpuTableR r, const uint8_t value) {
    switch (r) {
        case CPU_R_B:
            gb->cpu.b = value;
            break;
        case CPU_R_C:
            gb->cpu.c = value;
            break;
        case CPU_R_D:
            gb->cpu.d = value;
            break;
        case CPU_R_E:
            gb->cpu.e = value;
            break;
        case CPU_R_H:
            gb->cpu.h = value;
            break;
        case CPU_R_L:
            gb->cpu.l = value;
            break;
        case CPU_R_HL:
            GameBoy_write_mem(gb, Cpu_read_rp(&gb->cpu, CPU_RP_HL), value);
            break;
        case CPU_R_A:
            gb->cpu.a = value;
            break;
        default:
            BAIL("[GameBoy_write_r] invalid CpuTableR r: %i", r);
    }
}

void GameBoy_tick(GameBoy* const restrict gb) {
    const uint8_t opcode = GameBoy_read_pc(gb);
    // SDL_Log("tick (pc = 0x%04X opcode = 0x%02X)", gb->cpu.pc - 1, opcode);
    GameBoy_execute(gb, opcode);
}

void GameBoy_interrupt(GameBoy* const restrict gb, const uint8_t handler_location) {
    GameBoy_stack_push_u16(gb, gb->cpu.pc);
    gb->cpu.ime = false;
    gb->cpu.pc = handler_location;
    gb->cycle_count += 2;
}

void GameBoy_execute(GameBoy* const gb, const uint8_t opcode) {
    // https://archive.gbdev.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
    const uint8_t x = opcode >> 6;
    const uint8_t y = (opcode >> 3) & 0x7;
    const uint8_t z = opcode & 0x7;
    const uint8_t p = y >> 1;
    const uint8_t q = y & 1;

    switch (x) {
        case 0: {
            switch (z) {
                case 0: {
                    switch (y) {
                        case 0: {  // NOP
                            break;
                        }
                        case 1: {  // LD [n16], SP
                            const uint16_t addr = GameBoy_read_pc_u16(gb);
                            GameBoy_write_mem_u16(gb, addr, gb->cpu.sp);
                            break;
                        }
                        case 2: {  // STOP
                            BAIL("TODO: implement STOP instruction");
                            break;
                        }
                        case 3: {  // JR e8
                            gb->cpu.pc += (int8_t)GameBoy_read_pc(gb);
                            gb->cycle_count++;
                            break;
                        }
                        default: {  // JR cc, n16
                            const int8_t offset = GameBoy_read_pc(gb);
                            if (Cpu_read_cc(&gb->cpu, y - 4)) {
                                gb->cpu.pc += offset;
                                gb->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 1: {
                    if (q == 0) {  // LD r16, n16
                        const uint16_t value = GameBoy_read_pc_u16(gb);
                        Cpu_write_rp(&gb->cpu, p, value);
                    } else {  // ADD HL, r16
                        const uint16_t hl = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                        const uint16_t rhs = Cpu_read_rp(&gb->cpu, p);

                        Cpu_write_rp(&gb->cpu, p, hl + rhs);
                        Cpu_set_flag(&gb->cpu, CPU_FLAG_N, true);
                        Cpu_set_flag(
                            &gb->cpu, CPU_FLAG_H, (((hl & 0xFFF) + (rhs & 0xFFF)) & 0x1000) != 0
                        );
                        Cpu_set_flag(&gb->cpu, CPU_FLAG_C, rhs > 0xFFFF - hl);

                        gb->cycle_count++;
                    }
                    break;
                }
                case 2: {
                    if (q == 0) {
                        switch (p) {
                            case 0: {  // LD [BC], A
                                const uint16_t bc = Cpu_read_rp(&gb->cpu, CPU_RP_BC);
                                GameBoy_write_mem(gb, bc, gb->cpu.a);
                                break;
                            }
                            case 1: {  // LD [DE], A
                                const uint16_t de = Cpu_read_rp(&gb->cpu, CPU_RP_DE);
                                GameBoy_write_mem(gb, de, gb->cpu.a);
                                break;
                            }
                            case 2: {  // LD [HL+], A
                                const uint16_t hl = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                GameBoy_write_mem(gb, hl, gb->cpu.a);
                                Cpu_write_rp(&gb->cpu, CPU_RP_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD [HL-], A
                                const uint16_t hl = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                GameBoy_write_mem(gb, hl, gb->cpu.a);
                                Cpu_write_rp(&gb->cpu, CPU_RP_HL, hl - 1);
                                break;
                            }
                            default: {
                                BAIL("unreachable");
                            }
                        }
                    } else {
                        switch (p) {
                            case 0: {  // LD A, [BC]
                                const uint16_t bc = Cpu_read_rp(&gb->cpu, CPU_RP_BC);
                                gb->cpu.a = GameBoy_read_mem(gb, bc);
                                break;
                            }
                            case 1: {  // LD A, [DE]
                                const uint16_t de = Cpu_read_rp(&gb->cpu, CPU_RP_DE);
                                gb->cpu.a = GameBoy_read_mem(gb, de);
                                break;
                            }
                            case 2: {  // LD A, [HL+]
                                const uint16_t hl = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                gb->cpu.a = GameBoy_read_mem(gb, hl);
                                Cpu_write_rp(&gb->cpu, CPU_RP_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD A, [HL-]
                                const uint16_t hl = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                gb->cpu.a = GameBoy_read_mem(gb, hl);
                                Cpu_write_rp(&gb->cpu, CPU_RP_HL, hl - 1);
                                break;
                            }
                            default: {
                                BAIL("unreachable");
                            }
                        }
                    }
                    break;
                }
                case 3: {
                    if (q == 0) {  // INC r16
                        const uint16_t rp_value = Cpu_read_rp(&gb->cpu, p);
                        Cpu_write_rp(&gb->cpu, p, rp_value + 1);
                    } else {  // DEC r16
                        const uint16_t rp_value = Cpu_read_rp(&gb->cpu, p);
                        Cpu_write_rp(&gb->cpu, p, rp_value - 1);
                    }

                    gb->cycle_count++;
                    break;
                }
                case 4: {  // INC r8
                    const uint8_t value = GameBoy_read_r(gb, y);
                    const uint8_t new_value = value + 1;
                    GameBoy_write_r(gb, y, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, (value & 0xF) == 0xF);
                    break;
                }
                case 5: {  // DEC r8
                    const uint8_t value = GameBoy_read_r(gb, y);
                    const uint8_t new_value = value - 1;
                    GameBoy_write_r(gb, y, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, true);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, (value & 0xF) == 0);
                    break;
                }
                case 6: {  // LD r[y], n
                    const uint8_t value = GameBoy_read_pc(gb);
                    GameBoy_write_r(gb, y, value);
                    break;
                }
                case 7: {
                    switch (y) {
                        case 0: {  // RLCA
                            bool bit_7 = (gb->cpu.a & (1 << 7)) != 0;
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, bit_7);
                            gb->cpu.a = (gb->cpu.a << 1) | bit_7;
                            break;
                        }
                        case 1: {  // RRCA
                            bool bit_7 = gb->cpu.a & 1;
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, bit_7);
                            gb->cpu.a = (gb->cpu.a >> 1) | (bit_7 << 7);
                            break;
                        }
                        case 2: {  // RLA
                            bool prev_carry = (gb->cpu.f & CPU_FLAG_C) != 0;
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, (gb->cpu.a & (1 << 7)) != 0);
                            gb->cpu.a = (gb->cpu.a << 1) | prev_carry;
                            break;
                        }
                        case 3: {  // RRA
                            bool prev_carry = (gb->cpu.f & CPU_FLAG_C) != 0;
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, gb->cpu.a & 1);
                            gb->cpu.a = (gb->cpu.a >> 1) | (prev_carry << 7);
                            break;
                        }
                        case 4: {  // DAA
                            uint8_t adj = 0;

                            if (gb->cpu.f & CPU_FLAG_N) {
                                if (gb->cpu.f & CPU_FLAG_H) {
                                    adj += 0x06;
                                }

                                if (gb->cpu.f & CPU_FLAG_C) {
                                    adj += 0x60;
                                }

                                gb->cpu.a -= adj;
                            } else {
                                if (gb->cpu.f & CPU_FLAG_H || (gb->cpu.a & 0xF) > 0x9) {
                                    adj += 0x06;
                                }

                                if (gb->cpu.f & CPU_FLAG_C || gb->cpu.a > 0x99) {
                                    adj += 0x60;
                                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, true);
                                }

                                gb->cpu.a += adj;
                            }

                            Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, gb->cpu.a == 0);
                            break;
                        }
                        case 5: {  // CPL
                            gb->cpu.a = ~gb->cpu.a;
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            break;
                        }
                        case 6: {  // SCF
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, true);
                            break;
                        }
                        case 7: {  // CCF
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, !(gb->cpu.f & CPU_FLAG_C));
                            break;
                        }
                        default: {
                            BAIL("unreachable");
                        }
                    }
                    break;
                }
                default: {
                    BAIL("unreachable");
                }
            }
            break;
        }
        case 1: {
            if (z == 6 && y == 6) {  // HALT
                gb->cpu.halted = true;
            } else {  // LD r8, r8
                GameBoy_write_r(gb, y, GameBoy_read_r(gb, z));
            }
            break;
        }
        case 2: {  // {alu} A, r8
            const uint8_t rhs = GameBoy_read_r(gb, z);
            Cpu_alu(&gb->cpu, y, rhs);
            break;
        }
        case 3: {
            switch (z) {
                case 0: {
                    switch (y) {
                        case 4: {  // LDH [a8], A
                            const uint16_t addr = 0xFF00 + GameBoy_read_pc(gb);
                            GameBoy_write_mem(gb, addr, gb->cpu.a);
                            break;
                        }
                        case 5: {  // ADD SP, e8
                            const int8_t offset = GameBoy_read_pc(gb);

                            Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, false);
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                            Cpu_set_flag(
                                &gb->cpu,
                                CPU_FLAG_H,
                                (((gb->cpu.sp & 0xFFF) + offset) & 0x1000) != 0
                            );
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, offset > (0xFFFF - gb->cpu.sp));

                            gb->cpu.sp += offset;
                            gb->cycle_count += 2;
                            break;
                        }
                        case 6: {  // LDH A, [e8]
                            const uint16_t addr = 0xFF00 + GameBoy_read_pc(gb);
                            gb->cpu.a = GameBoy_read_mem(gb, addr);
                            break;
                        }
                        case 7: {  // LD HL, SP+e8
                            const int8_t offset = GameBoy_read_pc(gb);

                            Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, false);
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                            Cpu_set_flag(
                                &gb->cpu,
                                CPU_FLAG_H,
                                (((gb->cpu.sp & 0xFFF) + offset) & 0x1000) != 0
                            );
                            Cpu_set_flag(&gb->cpu, CPU_FLAG_C, offset > (0xFFFF - gb->cpu.sp));

                            Cpu_write_rp(&gb->cpu, CPU_RP_HL, gb->cpu.sp);
                            gb->cycle_count++;
                            break;
                        }
                        default: {  // RET cc
                            gb->cycle_count++;
                            if (Cpu_read_cc(&gb->cpu, y)) {
                                gb->cpu.pc = GameBoy_stack_pop_u16(gb);
                                gb->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 1: {
                    if (q == 0) {  // POP r16
                        const uint16_t value = GameBoy_stack_pop_u16(gb);
                        Cpu_write_rp2(&gb->cpu, p, value);
                    } else {
                        switch (p) {
                            case 0: {  // RET
                                gb->cpu.pc = GameBoy_stack_pop_u16(gb);
                                gb->cycle_count++;
                                break;
                            }
                            case 1: {  // RETI
                                gb->cpu.ime = true;
                                gb->cpu.pc = GameBoy_stack_pop_u16(gb);
                                gb->cycle_count++;
                                break;
                            }
                            case 2: {  // JP HL
                                gb->cpu.pc = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                break;
                            }
                            case 3: {  // LD SP, HL
                                gb->cpu.sp = Cpu_read_rp(&gb->cpu, CPU_RP_HL);
                                gb->cycle_count++;
                                break;
                            }
                            default: {
                                BAIL("unreachable");
                            }
                        }
                    }
                    break;
                }
                case 2: {
                    switch (y) {
                        case 4: {  // LDH [C], A
                            const uint16_t addr = 0xFF00 + gb->cpu.c;
                            GameBoy_write_mem(gb, addr, gb->cpu.a);
                            break;
                        }
                        case 5: {  // LD [a16], A
                            const uint16_t addr = GameBoy_read_pc_u16(gb);
                            GameBoy_write_mem(gb, addr, gb->cpu.a);
                            break;
                        }
                        case 6: {  // LDH A, [C]
                            const uint16_t addr = 0xFF00 + gb->cpu.c;
                            gb->cpu.a = GameBoy_read_mem(gb, addr);
                            break;
                        }
                        case 7: {  // LD A, [a16]
                            const uint16_t addr = GameBoy_read_pc_u16(gb);
                            gb->cpu.a = GameBoy_read_mem(gb, addr);
                            break;
                        }
                        default: {  // JP cc, a16
                            const uint16_t addr = GameBoy_read_pc_u16(gb);
                            if (Cpu_read_cc(&gb->cpu, y)) {
                                gb->cpu.pc = addr;
                                gb->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 3: {
                    switch (y) {
                        case 0: {  // JP a16
                            gb->cpu.pc = GameBoy_read_pc_u16(gb);
                            gb->cycle_count++;
                            break;
                        }
                        case 1: {  // PREFIX
                            const uint8_t sub_opcode = GameBoy_read_pc(gb);
                            GameBoy_execute_prefixed(gb, sub_opcode);
                            break;
                        }
                        case 6: {  // DI
                            gb->cpu.ime = false;
                            break;
                        }
                        case 7: {  // EI
                            gb->cpu.ime = true;
                            break;
                        }
                        default: {
                            BAIL("removed instruction");
                        }
                    }
                    break;
                }
                case 4: {
                    if (y < 4) {  // CALL cc, n16
                        const uint16_t addr = GameBoy_read_pc_u16(gb);
                        if (Cpu_read_cc(&gb->cpu, y)) {
                            GameBoy_stack_push_u16(gb, gb->cpu.pc);
                            gb->cpu.pc = addr;
                        }
                    } else {
                        BAIL("removed instruction");
                    }
                    break;
                }
                case 5: {
                    if (q == 0) {  // PUSH r16
                        const uint16_t value = Cpu_read_rp2(&gb->cpu, p);
                        GameBoy_stack_push_u16(gb, value);
                    } else if (p == 0) {  // CALL a16
                        const uint16_t addr = GameBoy_read_pc_u16(gb);
                        GameBoy_stack_push_u16(gb, gb->cpu.pc);
                        gb->cpu.pc = addr;
                    } else {
                        BAIL("removed instruction");
                    }
                    break;
                }
                case 6: {  // {alu} A, a8
                    const uint8_t rhs = GameBoy_read_pc(gb);
                    Cpu_alu(&gb->cpu, y, rhs);
                    break;
                }
                case 7: {  // RST vec
                    GameBoy_stack_push_u16(gb, gb->cpu.pc);
                    gb->cpu.pc = y * 8;
                    break;
                }
                default: {
                    BAIL("unreachable");
                }
            }
            break;
        }
        default: {
            BAIL("unreachable");
        }
    }
}

void GameBoy_execute_prefixed(GameBoy* restrict gb, const uint8_t opcode) {
    // SDL_Log("  prefixed (opcode = 0x%02X)", opcode);
    const uint8_t x = opcode >> 6;
    const uint8_t y = (opcode >> 3) & 0x7;
    const uint8_t z = opcode & 0x7;

    switch (x) {
        case 0: {
            switch (y) {
                case 0: {  // RLC r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool carry = (value & 0x80) != 0;
                    const uint8_t new_value = (value << 1) | carry;
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 1: {  // RRC r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (carry << 7);
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 2: {  // RL r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool prev_carry = (gb->cpu.f & CPU_FLAG_C) != 0;
                    const bool new_carry = (value & 0x80) != 0;
                    const uint8_t new_value = (value << 1) | prev_carry;
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, new_carry);
                    break;
                }
                case 3: {  // RR r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool prev_carry = (gb->cpu.f & CPU_FLAG_C) != 0;
                    const bool new_carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (prev_carry << 7);
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, new_carry);
                    break;
                }
                case 4: {  // SLA r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool carry = (value & 0x80) != 0;
                    const uint8_t new_value = value << 1;
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 5: {  // SRA r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool carry = value & 1;
                    const uint8_t b7 = value & 0x80;
                    const uint8_t new_value = (value >> 1) | b7;
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 6: {  // SWAP r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const uint8_t new_value = CONCAT_U16(LO(value), HI(value));
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, false);
                    break;
                }
                case 7: {  // SRL r8
                    const uint8_t value = GameBoy_read_r(gb, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = value >> 1;
                    GameBoy_write_r(gb, z, new_value);

                    Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(&gb->cpu, CPU_FLAG_C, carry);
                    break;
                }
                default: {
                    BAIL("unreachable");
                }
            }
            break;
        }
        case 1: {  // BIT u3, r8
            const uint8_t value = GameBoy_read_r(gb, z);
            Cpu_set_flag(&gb->cpu, CPU_FLAG_Z, (value & (1 << y)) == 0);
            Cpu_set_flag(&gb->cpu, CPU_FLAG_N, true);
            Cpu_set_flag(&gb->cpu, CPU_FLAG_H, false);
            break;
        }
        case 2: {  // RES u3, r8
            const uint8_t value = GameBoy_read_r(gb, z);
            GameBoy_write_r(gb, z, value & ~(1 << y));
            break;
        }
        case 3: {  // SET u3, r8
            const uint8_t value = GameBoy_read_r(gb, z);
            GameBoy_write_r(gb, z, value | (1 << y));
            break;
        }
        default: {
            BAIL("unreachable");
        }
    }
}
