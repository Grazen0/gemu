#include "cpu.h"
#include <stdint.h>
#include <stdlib.h>
#include "common/control.h"
#include "common/log.h"
#include "common/num.h"

Cpu Cpu_new(void) {
    return (Cpu){
        .b = 0,
        .c = 0,
        .d = 0,
        .e = 0,
        .h = 0,
        .l = 0,
        .a = 0,
        .f = 0,
        .pc = 0,
        .sp = 0,
        .halted = false,
        .ime = true,
        .cycle_count = 0,
    };
}

bool Cpu_read_cc(const Cpu* const restrict cpu, const CpuTableCc cc) {
    switch (cc) {
        case CpuTableCc_NZ:
            return (cpu->f & CpuFlag_Z) == 0;
        case CpuTableCc_Z:
            return (cpu->f & CpuFlag_Z) != 0;
        case CpuTableCc_NC:
            return (cpu->f & CpuFlag_C) == 0;
        case CpuTableCc_C:
            return (cpu->f & CpuFlag_C) != 0;
        default:
            BAIL("invalid cc: %i", cc);
    }
}

uint16_t Cpu_read_rp(const Cpu* const restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CpuTableRp_BC:
            return concat_u16(cpu->b, cpu->c);
        case CpuTableRp_DE:
            return concat_u16(cpu->d, cpu->e);
        case CpuTableRp_HL:
            return concat_u16(cpu->h, cpu->l);
        case CpuTableRp_SP:
            return cpu->sp;
        default:
            BAIL("invalid rp: %i", rp);
    }
}

void Cpu_write_rp(Cpu* const restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CpuTableRp_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            break;
        case CpuTableRp_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            break;
        case CpuTableRp_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            break;
        case CpuTableRp_SP:
            cpu->sp = value;
            break;
        default:
            BAIL("invalid rp: %i", rp);
    }
}

uint16_t Cpu_read_rp2(const Cpu* const restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CpuTableRp2_BC:
            return concat_u16(cpu->b, cpu->c);
        case CpuTableRp2_DE:
            return concat_u16(cpu->d, cpu->e);
        case CpuTableRp2_HL:
            return concat_u16(cpu->h, cpu->l);
        case CpuTableRp2_AF:
            return concat_u16(cpu->a, cpu->f);
        default:
            BAIL("invalid rp2: %i", rp);
    }
}

void Cpu_write_rp2(Cpu* const restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CpuTableRp2_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            break;
        case CpuTableRp2_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            break;
        case CpuTableRp2_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            break;
        case CpuTableRp2_AF:
            cpu->a = value >> 8;
            cpu->f = value & 0xF0;
            break;
        default:
            BAIL("invalid rp2: %i", rp);
    }
}

uint8_t
Cpu_read_mem(Cpu* const restrict cpu, const Memory* const restrict mem, const uint16_t addr) {
    cpu->cycle_count++;
    return mem->read(mem->ctx, addr);
}

uint16_t
Cpu_read_mem_u16(Cpu* const restrict cpu, const Memory* const restrict mem, const uint16_t addr) {
    const uint8_t lo = Cpu_read_mem(cpu, mem, addr);
    const uint8_t hi = Cpu_read_mem(cpu, mem, addr + 1);
    return concat_u16(hi, lo);
}

void Cpu_write_mem(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const uint16_t addr,
    const uint8_t value
) {
    cpu->cycle_count++;
    mem->write(mem->ctx, addr, value);
}

void Cpu_write_mem_u16(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const uint16_t addr,
    const uint16_t value
) {
    Cpu_write_mem(cpu, mem, addr, value & 0xFF);
    Cpu_write_mem(cpu, mem, addr + 1, value >> 8);
}

uint8_t Cpu_read_pc(Cpu* const restrict cpu, const Memory* const restrict mem) {
    const uint8_t value = Cpu_read_mem(cpu, mem, cpu->pc);
    cpu->pc++;
    return value;
}

uint16_t Cpu_read_pc_u16(Cpu* const restrict cpu, const Memory* const restrict mem) {
    const uint16_t value = Cpu_read_mem_u16(cpu, mem, cpu->pc);
    cpu->pc += 2;
    return value;
}

void Cpu_stack_push_u16(Cpu* const restrict cpu, Memory* const restrict mem, const uint16_t value) {
    cpu->sp -= 2;
    Cpu_write_mem_u16(cpu, mem, cpu->sp, value);
    cpu->cycle_count++;
}

uint16_t Cpu_stack_pop_u16(Cpu* const restrict cpu, const Memory* restrict mem) {
    const uint16_t value = Cpu_read_mem_u16(cpu, mem, cpu->sp);
    cpu->sp += 2;
    return value;
}

uint8_t Cpu_read_r(Cpu* const restrict cpu, const Memory* const restrict mem, const CpuTableR r) {
    switch (r) {
        case CpuTableR_B:
            return cpu->b;
        case CpuTableR_C:
            return cpu->c;
        case CpuTableR_D:
            return cpu->d;
        case CpuTableR_E:
            return cpu->e;
        case CpuTableR_H:
            return cpu->h;
        case CpuTableR_L:
            return cpu->l;
        case CpuTableR_HL: {
            const uint16_t addr = Cpu_read_rp(cpu, CpuTableRp_HL);
            return Cpu_read_mem(cpu, mem, addr);
        }
        case CpuTableR_A:
            return cpu->a;
        default:
            BAIL("invalid cpu r: %i", r);
    }
}

void Cpu_write_r(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const CpuTableR r,
    const uint8_t value
) {
    switch (r) {
        case CpuTableR_B:
            cpu->b = value;
            break;
        case CpuTableR_C:
            cpu->c = value;
            break;
        case CpuTableR_D:
            cpu->d = value;
            break;
        case CpuTableR_E:
            cpu->e = value;
            break;
        case CpuTableR_H:
            cpu->h = value;
            break;
        case CpuTableR_L:
            cpu->l = value;
            break;
        case CpuTableR_HL: {
            const uint16_t addr = Cpu_read_rp(cpu, CpuTableRp_HL);
            Cpu_write_mem(cpu, mem, addr, value);
            break;
        }
        case CpuTableR_A:
            cpu->a = value;
            break;
        default:
            BAIL("[GameBoy_write_r] invalid CpuTableR r: %i", r);
    }
}

void Cpu_alu(Cpu* const restrict cpu, const CpuTableAlu alu, const uint8_t rhs) {
    switch (alu) {
        case CpuTableAlu_ADD: {
            const uint8_t prev_a = cpu->a;
            cpu->a += rhs;

            set_bits(&cpu->f, CpuFlag_C, rhs > (0xFF - prev_a));
            set_bits(&cpu->f, CpuFlag_H, (((prev_a & 0xF) + (rhs & 0xF)) & 0x10) != 0);
            set_bits(&cpu->f, CpuFlag_N, false);
            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            break;
        }
        case CpuTableAlu_ADC: {
            const uint8_t prev_a = cpu->a;
            const uint8_t carry = (cpu->f & CpuFlag_C) != 0;
            const uint16_t result = (uint16_t)prev_a + rhs + carry;
            cpu->a = (uint8_t)result;

            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, false);
            set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) + (rhs & 0xF) + carry > 0xF);
            set_bits(&cpu->f, CpuFlag_C, result > 0xFF);
            break;
        }
        case CpuTableAlu_SUB: {
            const uint8_t prev_a = cpu->a;
            cpu->a -= rhs;

            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, true);
            set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) - (rhs & 0xF) > 0xF);
            set_bits(&cpu->f, CpuFlag_C, rhs > prev_a);
            break;
        }
        case CpuTableAlu_SBC: {
            const uint8_t prev_a = cpu->a;
            const uint8_t borrow = (cpu->f & CpuFlag_C) != 0;
            const uint16_t result = prev_a - rhs - borrow;
            cpu->a = (uint8_t)result;

            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, true);
            set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) < (rhs & 0xF) + borrow);
            set_bits(&cpu->f, CpuFlag_C, prev_a < (uint16_t)rhs + borrow);
            break;
        }
        case CpuTableAlu_AND: {
            cpu->a &= rhs;
            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, false);
            set_bits(&cpu->f, CpuFlag_H, true);
            set_bits(&cpu->f, CpuFlag_C, false);
            break;
        }
        case CpuTableAlu_XOR: {
            cpu->a ^= rhs;
            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, false);
            set_bits(&cpu->f, CpuFlag_H, false);
            set_bits(&cpu->f, CpuFlag_C, false);
            break;
        }
        case CpuTableAlu_OR: {
            cpu->a |= rhs;
            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
            set_bits(&cpu->f, CpuFlag_N, false);
            set_bits(&cpu->f, CpuFlag_H, false);
            set_bits(&cpu->f, CpuFlag_C, false);
            break;
        }
        case CpuTableAlu_CP: {
            set_bits(&cpu->f, CpuFlag_Z, cpu->a == rhs);
            set_bits(&cpu->f, CpuFlag_N, true);
            set_bits(&cpu->f, CpuFlag_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            set_bits(&cpu->f, CpuFlag_C, cpu->a < rhs);
            break;
        }
        default: {
            BAIL("invalid alu: %i", alu);
        }
    }
}

void Cpu_tick(Cpu* const restrict cpu, Memory* const restrict mem) {
    if (cpu->halted) {
        cpu->cycle_count++;  // Makes the frontend work lmao
        return;
    }

    // if (cpu->pc == 0xC1B9) {
    //     log_error(LogCategory_KEEP, "TEST FAILED! ===================");
    //     exit(1);
    // }
    const uint8_t opcode = Cpu_read_pc(cpu, mem);
    // log_info(LogCategory_KEEP, "tick (pc = $%04X opcode = $%02X)", cpu->pc - 1, opcode);
    Cpu_execute(cpu, mem, opcode);
}

void Cpu_execute(Cpu* const restrict cpu, Memory* const restrict mem, const uint8_t opcode) {
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
                            log_info(LogCategory_INSTRUCTION, "nop");
                            break;
                        }
                        case 1: {  // LD [n16], SP
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ld [$%04X], SP", addr);

                            Cpu_write_mem_u16(cpu, mem, addr, cpu->sp);
                            break;
                        }
                        case 2: {  // STOP
                            log_info(LogCategory_INSTRUCTION, "stop");

                            Cpu_write_mem(cpu, mem, 0xFF04, 0);  // Reset DIV
                            BAIL("TODO: implement STOP instruction");
                            break;
                        }
                        case 3: {  // JR e8
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "jr %i", offset);

                            cpu->pc += offset;
                            cpu->cycle_count++;
                            break;
                        }
                        default: {  // JR cc, n16
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "jr cc(%i), %i", y - 4, offset);

                            if (Cpu_read_cc(cpu, y - 4)) {
                                cpu->pc += offset;
                                cpu->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 1: {
                    if (q == 0) {  // LD r16, n16
                        const uint16_t value = Cpu_read_pc_u16(cpu, mem);
                        log_info(LogCategory_INSTRUCTION, "ld rp(%d), $%04X", p, value);

                        Cpu_write_rp(cpu, p, value);
                    } else {  // ADD HL, r16
                        log_info(LogCategory_INSTRUCTION, "add hl, rp(%d)", p);

                        const uint16_t hl = Cpu_read_rp(cpu, CpuTableRp_HL);
                        const uint16_t rhs = Cpu_read_rp(cpu, p);

                        Cpu_write_rp(cpu, CpuTableRp_HL, hl + rhs);

                        set_bits(&cpu->f, CpuFlag_N, false);
                        set_bits(&cpu->f, CpuFlag_H, (hl & 0xFFF) + (rhs & 0xFFF) > 0xFFF);
                        set_bits(&cpu->f, CpuFlag_C, rhs > 0xFFFF - hl);

                        cpu->cycle_count++;
                    }
                    break;
                }
                case 2: {
                    if (q == 0) {
                        switch (p) {
                            case 0: {  // LD [BC], A
                                log_info(LogCategory_INSTRUCTION, "ld [bc], a");

                                const uint16_t bc = Cpu_read_rp(cpu, CpuTableRp_BC);
                                Cpu_write_mem(cpu, mem, bc, cpu->a);
                                break;
                            }
                            case 1: {  // LD [DE], A
                                log_info(LogCategory_INSTRUCTION, "ld [de], a");

                                const uint16_t de = Cpu_read_rp(cpu, CpuTableRp_DE);
                                Cpu_write_mem(cpu, mem, de, cpu->a);
                                break;
                            }
                            case 2: {  // LD [HL+], A
                                log_info(LogCategory_INSTRUCTION, "ld [hl+], a");

                                const uint16_t hl = Cpu_read_rp(cpu, CpuTableRp_HL);
                                Cpu_write_mem(cpu, mem, hl, cpu->a);
                                Cpu_write_rp(cpu, CpuTableRp_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD [HL-], A
                                log_info(LogCategory_INSTRUCTION, "ld [hl-], a");

                                const uint16_t hl = Cpu_read_rp(cpu, CpuTableRp_HL);
                                Cpu_write_mem(cpu, mem, hl, cpu->a);
                                Cpu_write_rp(cpu, CpuTableRp_HL, hl - 1);
                                break;
                            }
                            default: {
                                BAIL("unreachable");
                            }
                        }
                    } else {
                        switch (p) {
                            case 0: {  // LD A, [BC]
                                log_info(LogCategory_INSTRUCTION, "ld a, [bc]");
                                const uint16_t bc = Cpu_read_rp(cpu, CpuTableRp_BC);
                                cpu->a = Cpu_read_mem(cpu, mem, bc);
                                break;
                            }
                            case 1: {  // LD A, [DE]
                                log_info(LogCategory_INSTRUCTION, "ld a, [de]");

                                const uint16_t de = Cpu_read_rp(cpu, CpuTableRp_DE);
                                cpu->a = Cpu_read_mem(cpu, mem, de);
                                break;
                            }
                            case 2: {  // LD A, [HL+]
                                log_info(LogCategory_INSTRUCTION, "ld a, [hl+]");

                                const uint16_t hl = Cpu_read_rp(cpu, CpuTableRp_HL);
                                cpu->a = Cpu_read_mem(cpu, mem, hl);
                                Cpu_write_rp(cpu, CpuTableRp_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD A, [HL-]
                                log_info(LogCategory_INSTRUCTION, "ld a, [hl-]");

                                const uint16_t hl = Cpu_read_rp(cpu, CpuTableRp_HL);
                                cpu->a = Cpu_read_mem(cpu, mem, hl);
                                Cpu_write_rp(cpu, CpuTableRp_HL, hl - 1);
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
                        log_info(LogCategory_INSTRUCTION, "inc rp(%d)", p);

                        const uint16_t value = Cpu_read_rp(cpu, p);
                        Cpu_write_rp(cpu, p, value + 1);
                    } else {  // DEC r16
                        log_info(LogCategory_INSTRUCTION, "dec rp(%d)", p);

                        const uint16_t value = Cpu_read_rp(cpu, p);
                        Cpu_write_rp(cpu, p, value - 1);
                    }

                    cpu->cycle_count++;
                    break;
                }
                case 4: {  // INC r8
                    log_info(LogCategory_INSTRUCTION, "inc r(%d)", y);

                    const uint8_t prev_value = Cpu_read_r(cpu, mem, y);
                    const uint8_t new_value = prev_value + 1;
                    Cpu_write_r(cpu, mem, y, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, (prev_value & 0xF) == 0xF);
                    break;
                }
                case 5: {  // DEC r8
                    log_info(LogCategory_INSTRUCTION, "dec r(%d)", y);

                    const uint8_t value = Cpu_read_r(cpu, mem, y);
                    const uint8_t new_value = value - 1;
                    Cpu_write_r(cpu, mem, y, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, true);
                    set_bits(&cpu->f, CpuFlag_H, (value & 0xF) == 0);
                    break;
                }
                case 6: {  // LD r8, n
                    const uint8_t value = Cpu_read_pc(cpu, mem);
                    log_info(LogCategory_INSTRUCTION, "ld r(%d), $%02X", y, value);

                    Cpu_write_r(cpu, mem, y, value);
                    break;
                }
                case 7: {
                    switch (y) {
                        case 0: {  // RLCA
                            log_info(LogCategory_INSTRUCTION, "rlca");

                            bool bit_7 = (cpu->a & (1 << 7)) != 0;
                            set_bits(&cpu->f, CpuFlag_C, bit_7);
                            cpu->a = (cpu->a << 1) | bit_7;
                            break;
                        }
                        case 1: {  // RRCA
                            log_info(LogCategory_INSTRUCTION, "rrca");

                            bool bit_7 = cpu->a & 1;
                            set_bits(&cpu->f, CpuFlag_C, bit_7);
                            cpu->a = (cpu->a >> 1) | (bit_7 << 7);
                            break;
                        }
                        case 2: {  // RLA
                            log_info(LogCategory_INSTRUCTION, "rla");

                            bool prev_carry = (cpu->f & CpuFlag_C) != 0;
                            set_bits(&cpu->f, CpuFlag_C, (cpu->a & (1 << 7)) != 0);
                            cpu->a = (cpu->a << 1) | prev_carry;
                            break;
                        }
                        case 3: {  // RRA
                            log_info(LogCategory_INSTRUCTION, "rra");

                            bool prev_carry = (cpu->f & CpuFlag_C) != 0;
                            set_bits(&cpu->f, CpuFlag_C, cpu->a & 1);
                            cpu->a = (cpu->a >> 1) | (prev_carry << 7);
                            break;
                        }
                        case 4: {  // DAA
                            log_info(LogCategory_INSTRUCTION, "daa");

                            uint8_t adj = 0;

                            if (cpu->f & CpuFlag_N) {
                                if (cpu->f & CpuFlag_H) {
                                    adj += 0x06;
                                }

                                if (cpu->f & CpuFlag_C) {
                                    adj += 0x60;
                                }

                                cpu->a -= adj;
                            } else {
                                if (cpu->f & CpuFlag_H || (cpu->a & 0xF) > 0x9) {
                                    adj += 0x06;
                                }

                                if (cpu->f & CpuFlag_C || cpu->a > 0x99) {
                                    adj += 0x60;
                                    set_bits(&cpu->f, CpuFlag_C, true);
                                }

                                cpu->a += adj;
                            }

                            set_bits(&cpu->f, CpuFlag_H, false);
                            set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
                            break;
                        }
                        case 5: {  // CPL
                            log_info(LogCategory_INSTRUCTION, "daa");

                            cpu->a = ~cpu->a;
                            set_bits(&cpu->f, CpuFlag_N | CpuFlag_H, false);
                            break;
                        }
                        case 6: {  // SCF
                            log_info(LogCategory_INSTRUCTION, "scf");

                            set_bits(&cpu->f, CpuFlag_N, false);
                            set_bits(&cpu->f, CpuFlag_H, false);
                            set_bits(&cpu->f, CpuFlag_C, true);
                            break;
                        }
                        case 7: {  // CCF
                            log_info(LogCategory_INSTRUCTION, "ccf");

                            set_bits(&cpu->f, CpuFlag_N, false);
                            set_bits(&cpu->f, CpuFlag_H, false);
                            set_bits(&cpu->f, CpuFlag_C, !(cpu->f & CpuFlag_C));
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
                log_info(LogCategory_INSTRUCTION, "halt");
                cpu->halted = true;
            } else {  // LD r8, r8
                const uint8_t value = Cpu_read_r(cpu, mem, z);
                log_info(LogCategory_INSTRUCTION, "ld r(%d), r(%d)", y, z);

                Cpu_write_r(cpu, mem, y, value);
            }
            break;
        }
        case 2: {  // {alu} A, r8
            log_info(LogCategory_INSTRUCTION, "{alu} A, r(%d)", z);
            const uint8_t rhs = Cpu_read_r(cpu, mem, z);
            Cpu_alu(cpu, y, rhs);
            break;
        }
        case 3: {
            switch (z) {
                case 0: {
                    switch (y) {
                        case 4: {  // LDH [a8], A
                            const uint8_t offset = Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ldh [$%02X], a", offset);

                            const uint16_t addr = 0xFF00 + offset;
                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 5: {  // ADD SP, e8
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "add sp, %d", offset);

                            set_bits(&cpu->f, CpuFlag_Z, false);
                            set_bits(&cpu->f, CpuFlag_N, false);
                            set_bits(&cpu->f, CpuFlag_H, (cpu->sp & 0xF) + offset > 0xF);
                            set_bits(&cpu->f, CpuFlag_C, (cpu->sp & 0xFF) + offset > 0xFF);

                            cpu->sp += offset;
                            cpu->cycle_count += 2;
                            break;
                        }
                        case 6: {  // LDH A, [e8]
                            const uint8_t offset = Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ldh a, [$%02X]", offset);

                            const uint16_t addr = 0xFF00 + offset;
                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        case 7: {  // LD HL, SP+e8
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ld hl, sp%+d", offset);

                            set_bits(&cpu->f, CpuFlag_Z, false);
                            set_bits(&cpu->f, CpuFlag_N, false);
                            set_bits(&cpu->f, CpuFlag_H, (cpu->sp & 0xF) + offset > 0xF);
                            set_bits(&cpu->f, CpuFlag_C, (cpu->sp & 0xFF) + offset > 0xFF);

                            Cpu_write_rp(cpu, CpuTableRp_HL, cpu->sp + offset);
                            cpu->cycle_count++;
                            break;
                        }
                        default: {  // RET cc
                            log_info(LogCategory_INSTRUCTION, "ret cc(%d)", y);

                            cpu->cycle_count++;
                            if (Cpu_read_cc(cpu, y)) {
                                cpu->pc = Cpu_stack_pop_u16(cpu, mem);
                                cpu->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 1: {
                    if (q == 0) {  // POP r16
                        log_info(LogCategory_INSTRUCTION, "pop rp2(%d)", p);

                        const uint16_t value = Cpu_stack_pop_u16(cpu, mem);
                        Cpu_write_rp2(cpu, p, value);
                    } else {
                        switch (p) {
                            case 0: {  // RET
                                log_info(LogCategory_INSTRUCTION, "ret");
                                cpu->pc = Cpu_stack_pop_u16(cpu, mem);
                                cpu->cycle_count++;
                                break;
                            }
                            case 1: {  // RETI
                                log_info(LogCategory_INSTRUCTION, "reti");

                                cpu->ime = true;
                                cpu->pc = Cpu_stack_pop_u16(cpu, mem);
                                cpu->cycle_count++;
                                break;
                            }
                            case 2: {  // JP HL
                                log_info(LogCategory_INSTRUCTION, "jp hl");
                                cpu->pc = Cpu_read_rp(cpu, CpuTableRp_HL);
                                break;
                            }
                            case 3: {  // LD SP, HL
                                log_info(LogCategory_INSTRUCTION, "ld sp, hl");

                                cpu->sp = Cpu_read_rp(cpu, CpuTableRp_HL);
                                cpu->cycle_count++;
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
                            log_info(LogCategory_INSTRUCTION, "ldh [c], a");

                            const uint16_t addr = 0xFF00 + cpu->c;
                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 5: {  // LD [a16], A
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ld [$%04X], a", addr);

                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 6: {  // LDH A, [C]
                            const uint16_t addr = 0xFF00 + cpu->c;
                            log_info(LogCategory_INSTRUCTION, "ld a, [c]");

                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        case 7: {  // LD A, [a16]
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "ld a, [$%04X]", addr);

                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        default: {  // JP cc, a16
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "jp cc(%d), $%04X", y, addr);

                            if (Cpu_read_cc(cpu, y)) {
                                cpu->pc = addr;
                                cpu->cycle_count++;
                            }
                        }
                    }
                    break;
                }
                case 3: {
                    switch (y) {
                        case 0: {  // JP a16
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "jp $%04X", addr);

                            cpu->pc = addr;
                            cpu->cycle_count++;
                            break;
                        }
                        case 1: {  // PREFIX
                            const uint8_t sub_opcode = Cpu_read_pc(cpu, mem);
                            log_info(LogCategory_INSTRUCTION, "{prefix} $%02X", sub_opcode);

                            Cpu_execute_prefixed(cpu, mem, sub_opcode);
                            break;
                        }
                        case 6: {  // DI
                            log_info(LogCategory_INSTRUCTION, "di");
                            cpu->ime = false;
                            break;
                        }
                        case 7: {  // EI
                            log_info(LogCategory_INSTRUCTION, "ei");
                            cpu->ime = true;
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
                        const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                        log_info(LogCategory_INSTRUCTION, "call cc(%d), $%04X", y, addr);

                        if (Cpu_read_cc(cpu, y)) {
                            Cpu_stack_push_u16(cpu, mem, cpu->pc);
                            cpu->pc = addr;
                        }
                    } else {
                        BAIL("removed instruction");
                    }
                    break;
                }
                case 5: {
                    if (q == 0) {  // PUSH r16
                        log_info(LogCategory_INSTRUCTION, "push rp2(%d)", p);

                        const uint16_t value = Cpu_read_rp2(cpu, p);
                        Cpu_stack_push_u16(cpu, mem, value);
                    } else if (p == 0) {  // CALL a16
                        const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                        Cpu_stack_push_u16(cpu, mem, cpu->pc);
                        cpu->pc = addr;
                    } else {
                        BAIL("removed instruction");
                    }
                    break;
                }
                case 6: {  // {alu} A, a8
                    const uint8_t rhs = Cpu_read_pc(cpu, mem);
                    log_info(LogCategory_INSTRUCTION, "{alu} a, $%02X", rhs);
                    Cpu_alu(cpu, y, rhs);
                    break;
                }
                case 7: {  // RST vec
                    log_info(LogCategory_INSTRUCTION, "rst $%02X", y * 8);
                    Cpu_stack_push_u16(cpu, mem, cpu->pc);
                    cpu->pc = y * 8;
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

void Cpu_execute_prefixed(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const uint8_t opcode
) {
    // log_info("  prefixed (opcode = 0x%02X)", opcode);
    const uint8_t x = opcode >> 6;
    const uint8_t y = (opcode >> 3) & 0x7;
    const uint8_t z = opcode & 0x7;

    switch (x) {
        case 0: {
            switch (y) {
                case 0: {  // RLC r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = (value & 0x80) != 0;
                    const uint8_t new_value = (value << 1) | carry;
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, carry);
                    break;
                }
                case 1: {  // RRC r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (carry << 7);
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, carry);
                    break;
                }
                case 2: {  // RL r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool prev_carry = (cpu->f & CpuFlag_C) != 0;
                    const bool new_carry = (value & 0x80) != 0;
                    const uint8_t new_value = (value << 1) | prev_carry;
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, new_carry);
                    break;
                }
                case 3: {  // RR r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool prev_carry = (cpu->f & CpuFlag_C) != 0;
                    const bool new_carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (prev_carry << 7);
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, new_carry);
                    break;
                }
                case 4: {  // SLA r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = (value & 0x80) != 0;
                    const uint8_t new_value = value << 1;
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, carry);
                    break;
                }
                case 5: {  // SRA r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t b7 = value & 0x80;
                    const uint8_t new_value = (value >> 1) | b7;
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, carry);
                    break;
                }
                case 6: {  // SWAP r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const uint8_t new_value = concat_u16(value & 0xFF, value >> 8);
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, false);
                    break;
                }
                case 7: {  // SRL r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = value >> 1;
                    Cpu_write_r(cpu, mem, z, new_value);

                    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
                    set_bits(&cpu->f, CpuFlag_N, false);
                    set_bits(&cpu->f, CpuFlag_H, false);
                    set_bits(&cpu->f, CpuFlag_C, carry);
                    break;
                }
                default: {
                    BAIL("unreachable");
                }
            }
            break;
        }
        case 1: {  // BIT u3, r8
            const uint8_t value = Cpu_read_r(cpu, mem, z);
            set_bits(&cpu->f, CpuFlag_Z, (value & (1 << y)) == 0);
            set_bits(&cpu->f, CpuFlag_N, true);
            set_bits(&cpu->f, CpuFlag_H, false);
            break;
        }
        case 2: {  // RES u3, r8
            const uint8_t value = Cpu_read_r(cpu, mem, z);
            Cpu_write_r(cpu, mem, z, value & ~(1 << y));
            break;
        }
        case 3: {  // SET u3, r8
            const uint8_t value = Cpu_read_r(cpu, mem, z);
            Cpu_write_r(cpu, mem, z, value | (1 << y));
            break;
        }
        default: {
            BAIL("unreachable");
        }
    }
}

void Cpu_interrupt(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const uint8_t handler_location
) {
    Cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->ime = false;
    cpu->pc = handler_location;
    cpu->cycle_count += 2;
}
