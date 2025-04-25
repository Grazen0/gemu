#include "cpu.h"
#include <stdint.h>
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

void Cpu_set_flag(Cpu* const restrict cpu, const CpuFlag flag, const bool value) {
    if (value) {
        cpu->f |= flag;
    } else {
        cpu->f &= ~flag;
    }
}

bool Cpu_read_cc(const Cpu* const restrict cpu, const CpuTableCc cc) {
    switch (cc) {
        case CPU_CC_NZ:
            return (cpu->f & CPU_FLAG_Z) == 0;
        case CPU_CC_Z:
            return (cpu->f & CPU_FLAG_Z) != 0;
        case CPU_CC_NC:
            return (cpu->f & CPU_FLAG_C) == 0;
        case CPU_CC_C:
            return (cpu->f & CPU_FLAG_C) != 0;
        default:
            bail("invalid cc: %i", cc);
    }
}

uint16_t Cpu_read_rp(const Cpu* const restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CPU_RP_BC:
            return concat_u16(cpu->b, cpu->c);
        case CPU_RP_DE:
            return concat_u16(cpu->d, cpu->e);
        case CPU_RP_HL:
            return concat_u16(cpu->h, cpu->l);
        case CPU_RP_SP:
            return cpu->sp;
        default:
            bail("invalid rp: %i", rp);
    }
}

void Cpu_write_rp(Cpu* const restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CPU_RP_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            break;
        case CPU_RP_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            break;
        case CPU_RP_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            break;
        case CPU_RP_SP:
            cpu->sp = value;
            break;
        default:
            bail("invalid rp: %i", rp);
    }
}

uint16_t Cpu_read_rp2(const Cpu* const restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CPU_RP2_BC:
            return concat_u16(cpu->b, cpu->c);
        case CPU_RP2_DE:
            return concat_u16(cpu->d, cpu->e);
        case CPU_RP2_HL:
            return concat_u16(cpu->h, cpu->l);
        case CPU_RP2_AF:
            return concat_u16(cpu->a, cpu->f);
        default:
            bail("invalid rp2: %i", rp);
    }
}

void Cpu_write_rp2(Cpu* const restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CPU_RP2_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            break;
        case CPU_RP2_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            break;
        case CPU_RP2_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            break;
        case CPU_RP2_AF:
            cpu->a = value >> 8;
            cpu->f = value & 0xFF;
            break;
        default:
            bail("invalid rp2: %i", rp);
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
    Cpu_write_mem(cpu, mem, cpu->sp - 1, value >> 8);
    Cpu_write_mem(cpu, mem, cpu->sp - 2, value & 0xFF);
    cpu->sp -= 2;
    cpu->cycle_count++;
}

uint16_t Cpu_stack_pop_u16(Cpu* const restrict cpu, Memory* restrict mem) {
    const uint8_t lo = Cpu_read_mem(cpu, mem, cpu->sp);
    const uint8_t hi = Cpu_read_mem(cpu, mem, cpu->sp + 1);
    cpu->sp += 2;
    return concat_u16(hi, lo);
}

uint8_t Cpu_read_r(Cpu* const restrict cpu, const Memory* const restrict mem, const CpuTableR r) {
    switch (r) {
        case CPU_R_B:
            return cpu->b;
        case CPU_R_C:
            return cpu->c;
        case CPU_R_D:
            return cpu->d;
        case CPU_R_E:
            return cpu->e;
        case CPU_R_H:
            return cpu->h;
        case CPU_R_L:
            return cpu->l;
        case CPU_R_HL: {
            const uint16_t addr = Cpu_read_rp(cpu, CPU_RP_HL);
            return Cpu_read_mem(cpu, mem, addr);
        }
        case CPU_R_A:
            return cpu->a;
        default:
            bail("invalid cpu r: %i", r);
    }
}

void Cpu_write_r(
    Cpu* const restrict cpu,
    Memory* const restrict mem,
    const CpuTableR r,
    const uint8_t value
) {
    switch (r) {
        case CPU_R_B:
            cpu->b = value;
            break;
        case CPU_R_C:
            cpu->c = value;
            break;
        case CPU_R_D:
            cpu->d = value;
            break;
        case CPU_R_E:
            cpu->e = value;
            break;
        case CPU_R_H:
            cpu->h = value;
            break;
        case CPU_R_L:
            cpu->l = value;
            break;
        case CPU_R_HL: {
            const uint16_t addr = Cpu_read_rp(cpu, CPU_RP_HL);
            Cpu_write_mem(cpu, mem, addr, value);
            break;
        }
        case CPU_R_A:
            cpu->a = value;
            break;
        default:
            bail("[GameBoy_write_r] invalid CpuTableR r: %i", r);
    }
}

void Cpu_alu(Cpu* const restrict cpu, const CpuTableAlu alu, const uint8_t rhs) {
    switch (alu) {
        case CPU_ALU_ADD: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > (0xFF - cpu->a));
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) + (rhs & 0xF)) & 0x10) != 0);
            cpu->a += rhs;
            cpu->f &= ~CPU_FLAG_N;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_ADC: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > (0xFF - cpu->a));
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) + (rhs & 0xF)) & 0x10) != 0);
            cpu->a += rhs;

            if (cpu->f & CPU_FLAG_C) {
                if (cpu->a == 0xFF) {
                    cpu->f |= CPU_FLAG_C;
                }

                if ((cpu->a & 0xF) == 0xF) {
                    cpu->f |= CPU_FLAG_H;
                }

                cpu->a++;
            }

            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            cpu->f &= ~CPU_FLAG_N;
            break;
        }
        case CPU_ALU_SUB: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            cpu->a -= rhs;
            cpu->f |= CPU_FLAG_N;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_SBC: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            cpu->a -= rhs;

            if (cpu->f & CPU_FLAG_C) {
                if (cpu->a == 0) {
                    cpu->f |= CPU_FLAG_C;
                }

                if ((cpu->a & 0xF) == 0) {
                    cpu->f &= ~CPU_FLAG_H;
                }

                cpu->a--;
            }

            cpu->f |= CPU_FLAG_N;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_AND: {
            cpu->a &= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            cpu->f &= ~CPU_FLAG_N | ~CPU_FLAG_C;
            cpu->f |= CPU_FLAG_H;
            break;
        }
        case CPU_ALU_XOR: {
            cpu->a ^= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            cpu->f &= ~CPU_FLAG_N | ~CPU_FLAG_H | ~CPU_FLAG_C;
            break;
        }
        case CPU_ALU_OR: {
            cpu->a |= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            cpu->f &= ~CPU_FLAG_N | ~CPU_FLAG_H | ~CPU_FLAG_C;
            break;
        }
        case CPU_ALU_CP: {
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == rhs);
            cpu->f |= CPU_FLAG_N;
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            break;
        }
        default: {
            bail("invalid alu: %i", alu);
        }
    }
}

void Cpu_tick(Cpu* const restrict cpu, Memory* const restrict mem) {
    const uint8_t opcode = Cpu_read_pc(cpu, mem);
    // log_info("tick (pc = 0x%04X opcode = 0x%02X)", cpu->pc - 1, opcode);
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
                            break;
                        }
                        case 1: {  // LD [n16], SP
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            Cpu_write_mem_u16(cpu, mem, addr, cpu->sp);
                            break;
                        }
                        case 2: {  // STOP
                            bail("TODO: implement STOP instruction");
                            break;
                        }
                        case 3: {  // JR e8
                            cpu->pc += (int8_t)Cpu_read_pc(cpu, mem);
                            cpu->cycle_count++;
                            break;
                        }
                        default: {  // JR cc, n16
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);
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
                        Cpu_write_rp(cpu, p, value);
                    } else {  // ADD HL, r16
                        const uint16_t hl = Cpu_read_rp(cpu, CPU_RP_HL);
                        const uint16_t rhs = Cpu_read_rp(cpu, p);

                        Cpu_write_rp(cpu, p, hl + rhs);
                        Cpu_set_flag(cpu, CPU_FLAG_N, true);
                        Cpu_set_flag(
                            cpu, CPU_FLAG_H, (((hl & 0xFFF) + (rhs & 0xFFF)) & 0x1000) != 0
                        );
                        Cpu_set_flag(cpu, CPU_FLAG_C, rhs > 0xFFFF - hl);

                        cpu->cycle_count++;
                    }
                    break;
                }
                case 2: {
                    if (q == 0) {
                        switch (p) {
                            case 0: {  // LD [BC], A
                                const uint16_t bc = Cpu_read_rp(cpu, CPU_RP_BC);
                                Cpu_write_mem(cpu, mem, bc, cpu->a);
                                break;
                            }
                            case 1: {  // LD [DE], A
                                const uint16_t de = Cpu_read_rp(cpu, CPU_RP_DE);
                                Cpu_write_mem(cpu, mem, de, cpu->a);
                                break;
                            }
                            case 2: {  // LD [HL+], A
                                const uint16_t hl = Cpu_read_rp(cpu, CPU_RP_HL);
                                Cpu_write_mem(cpu, mem, hl, cpu->a);
                                Cpu_write_rp(cpu, CPU_RP_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD [HL-], A
                                const uint16_t hl = Cpu_read_rp(cpu, CPU_RP_HL);
                                Cpu_write_mem(cpu, mem, hl, cpu->a);
                                Cpu_write_rp(cpu, CPU_RP_HL, hl - 1);
                                break;
                            }
                            default: {
                                bail("unreachable");
                            }
                        }
                    } else {
                        switch (p) {
                            case 0: {  // LD A, [BC]
                                const uint16_t bc = Cpu_read_rp(cpu, CPU_RP_BC);
                                cpu->a = Cpu_read_mem(cpu, mem, bc);
                                break;
                            }
                            case 1: {  // LD A, [DE]
                                const uint16_t de = Cpu_read_rp(cpu, CPU_RP_DE);
                                cpu->a = Cpu_read_mem(cpu, mem, de);
                                break;
                            }
                            case 2: {  // LD A, [HL+]
                                const uint16_t hl = Cpu_read_rp(cpu, CPU_RP_HL);
                                cpu->a = Cpu_read_mem(cpu, mem, hl);
                                Cpu_write_rp(cpu, CPU_RP_HL, hl + 1);
                                break;
                            }
                            case 3: {  // LD A, [HL-]
                                const uint16_t hl = Cpu_read_rp(cpu, CPU_RP_HL);
                                cpu->a = Cpu_read_mem(cpu, mem, hl);
                                Cpu_write_rp(cpu, CPU_RP_HL, hl - 1);
                                break;
                            }
                            default: {
                                bail("unreachable");
                            }
                        }
                    }
                    break;
                }
                case 3: {
                    if (q == 0) {  // INC r16
                        const uint16_t rp_value = Cpu_read_rp(cpu, p);
                        Cpu_write_rp(cpu, p, rp_value + 1);
                    } else {  // DEC r16
                        const uint16_t rp_value = Cpu_read_rp(cpu, p);
                        Cpu_write_rp(cpu, p, rp_value - 1);
                    }

                    cpu->cycle_count++;
                    break;
                }
                case 4: {  // INC r8
                    const uint8_t value = Cpu_read_r(cpu, mem, y);
                    const uint8_t new_value = value + 1;
                    Cpu_write_r(cpu, mem, y, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, (value & 0xF) == 0xF);
                    break;
                }
                case 5: {  // DEC r8
                    const uint8_t value = Cpu_read_r(cpu, mem, y);
                    const uint8_t new_value = value - 1;
                    Cpu_write_r(cpu, mem, y, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, true);
                    Cpu_set_flag(cpu, CPU_FLAG_H, (value & 0xF) == 0);
                    break;
                }
                case 6: {  // LD r[y], n
                    const uint8_t value = Cpu_read_pc(cpu, mem);
                    Cpu_write_r(cpu, mem, y, value);
                    break;
                }
                case 7: {
                    switch (y) {
                        case 0: {  // RLCA
                            bool bit_7 = (cpu->a & (1 << 7)) != 0;
                            Cpu_set_flag(cpu, CPU_FLAG_C, bit_7);
                            cpu->a = (cpu->a << 1) | bit_7;
                            break;
                        }
                        case 1: {  // RRCA
                            bool bit_7 = cpu->a & 1;
                            Cpu_set_flag(cpu, CPU_FLAG_C, bit_7);
                            cpu->a = (cpu->a >> 1) | (bit_7 << 7);
                            break;
                        }
                        case 2: {  // RLA
                            bool prev_carry = (cpu->f & CPU_FLAG_C) != 0;
                            Cpu_set_flag(cpu, CPU_FLAG_C, (cpu->a & (1 << 7)) != 0);
                            cpu->a = (cpu->a << 1) | prev_carry;
                            break;
                        }
                        case 3: {  // RRA
                            bool prev_carry = (cpu->f & CPU_FLAG_C) != 0;
                            Cpu_set_flag(cpu, CPU_FLAG_C, cpu->a & 1);
                            cpu->a = (cpu->a >> 1) | (prev_carry << 7);
                            break;
                        }
                        case 4: {  // DAA
                            uint8_t adj = 0;

                            if (cpu->f & CPU_FLAG_N) {
                                if (cpu->f & CPU_FLAG_H) {
                                    adj += 0x06;
                                }

                                if (cpu->f & CPU_FLAG_C) {
                                    adj += 0x60;
                                }

                                cpu->a -= adj;
                            } else {
                                if (cpu->f & CPU_FLAG_H || (cpu->a & 0xF) > 0x9) {
                                    adj += 0x06;
                                }

                                if (cpu->f & CPU_FLAG_C || cpu->a > 0x99) {
                                    adj += 0x60;
                                    Cpu_set_flag(cpu, CPU_FLAG_C, true);
                                }

                                cpu->a += adj;
                            }

                            Cpu_set_flag(cpu, CPU_FLAG_H, false);
                            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
                            break;
                        }
                        case 5: {  // CPL
                            cpu->a = ~cpu->a;
                            Cpu_set_flag(cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            break;
                        }
                        case 6: {  // SCF
                            Cpu_set_flag(cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            Cpu_set_flag(cpu, CPU_FLAG_C, true);
                            break;
                        }
                        case 7: {  // CCF
                            Cpu_set_flag(cpu, CPU_FLAG_N | CPU_FLAG_H, false);
                            Cpu_set_flag(cpu, CPU_FLAG_C, !(cpu->f & CPU_FLAG_C));
                            break;
                        }
                        default: {
                            bail("unreachable");
                        }
                    }
                    break;
                }
                default: {
                    bail("unreachable");
                }
            }
            break;
        }
        case 1: {
            if (z == 6 && y == 6) {  // HALT
                cpu->halted = true;
            } else {  // LD r8, r8
                const uint8_t value = Cpu_read_r(cpu, mem, z);
                Cpu_write_r(cpu, mem, y, value);
            }
            break;
        }
        case 2: {  // {alu} A, r8
            const uint8_t rhs = Cpu_read_r(cpu, mem, z);
            Cpu_alu(cpu, y, rhs);
            break;
        }
        case 3: {
            switch (z) {
                case 0: {
                    switch (y) {
                        case 4: {  // LDH [a8], A
                            const uint16_t addr = 0xFF00 + Cpu_read_pc(cpu, mem);
                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 5: {  // ADD SP, e8
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);

                            Cpu_set_flag(cpu, CPU_FLAG_Z, false);
                            Cpu_set_flag(cpu, CPU_FLAG_N, false);
                            Cpu_set_flag(
                                cpu, CPU_FLAG_H, (((cpu->sp & 0xFFF) + offset) & 0x1000) != 0
                            );
                            Cpu_set_flag(cpu, CPU_FLAG_C, offset > (0xFFFF - cpu->sp));

                            cpu->sp += offset;
                            cpu->cycle_count += 2;
                            break;
                        }
                        case 6: {  // LDH A, [e8]
                            const uint16_t addr = 0xFF00 + Cpu_read_pc(cpu, mem);
                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        case 7: {  // LD HL, SP+e8
                            const int8_t offset = (int8_t)Cpu_read_pc(cpu, mem);

                            Cpu_set_flag(cpu, CPU_FLAG_Z, false);
                            Cpu_set_flag(cpu, CPU_FLAG_N, false);
                            Cpu_set_flag(
                                cpu, CPU_FLAG_H, (((cpu->sp & 0xFFF) + offset) & 0x1000) != 0
                            );
                            Cpu_set_flag(cpu, CPU_FLAG_C, offset > (0xFFFF - cpu->sp));

                            Cpu_write_rp(cpu, CPU_RP_HL, cpu->sp);
                            cpu->cycle_count++;
                            break;
                        }
                        default: {  // RET cc
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
                        const uint16_t value = Cpu_stack_pop_u16(cpu, mem);
                        Cpu_write_rp2(cpu, p, value);
                    } else {
                        switch (p) {
                            case 0: {  // RET
                                cpu->pc = Cpu_stack_pop_u16(cpu, mem);
                                cpu->cycle_count++;
                                break;
                            }
                            case 1: {  // RETI
                                cpu->ime = true;
                                cpu->pc = Cpu_stack_pop_u16(cpu, mem);
                                cpu->cycle_count++;
                                break;
                            }
                            case 2: {  // JP HL
                                cpu->pc = Cpu_read_rp(cpu, CPU_RP_HL);
                                break;
                            }
                            case 3: {  // LD SP, HL
                                cpu->sp = Cpu_read_rp(cpu, CPU_RP_HL);
                                cpu->cycle_count++;
                                break;
                            }
                            default: {
                                bail("unreachable");
                            }
                        }
                    }
                    break;
                }
                case 2: {
                    switch (y) {
                        case 4: {  // LDH [C], A
                            const uint16_t addr = 0xFF00 + cpu->c;
                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 5: {  // LD [a16], A
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            Cpu_write_mem(cpu, mem, addr, cpu->a);
                            break;
                        }
                        case 6: {  // LDH A, [C]
                            const uint16_t addr = 0xFF00 + cpu->c;
                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        case 7: {  // LD A, [a16]
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                            cpu->a = Cpu_read_mem(cpu, mem, addr);
                            break;
                        }
                        default: {  // JP cc, a16
                            const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
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
                            cpu->pc = Cpu_read_pc_u16(cpu, mem);
                            cpu->cycle_count++;
                            break;
                        }
                        case 1: {  // PREFIX
                            const uint8_t sub_opcode = Cpu_read_pc(cpu, mem);
                            Cpu_execute_prefixed(cpu, mem, sub_opcode);
                            break;
                        }
                        case 6: {  // DI
                            cpu->ime = false;
                            break;
                        }
                        case 7: {  // EI
                            cpu->ime = true;
                            break;
                        }
                        default: {
                            bail("removed instruction");
                        }
                    }
                    break;
                }
                case 4: {
                    if (y < 4) {  // CALL cc, n16
                        const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                        if (Cpu_read_cc(cpu, y)) {
                            Cpu_stack_push_u16(cpu, mem, cpu->pc);
                            cpu->pc = addr;
                        }
                    } else {
                        bail("removed instruction");
                    }
                    break;
                }
                case 5: {
                    if (q == 0) {  // PUSH r16
                        const uint16_t value = Cpu_read_rp2(cpu, p);
                        Cpu_stack_push_u16(cpu, mem, value);
                    } else if (p == 0) {  // CALL a16
                        const uint16_t addr = Cpu_read_pc_u16(cpu, mem);
                        Cpu_stack_push_u16(cpu, mem, cpu->pc);
                        cpu->pc = addr;
                    } else {
                        bail("removed instruction");
                    }
                    break;
                }
                case 6: {  // {alu} A, a8
                    const uint8_t rhs = Cpu_read_pc(cpu, mem);
                    Cpu_alu(cpu, y, rhs);
                    break;
                }
                case 7: {  // RST vec
                    Cpu_stack_push_u16(cpu, mem, cpu->pc);
                    cpu->pc = y * 8;
                    break;
                }
                default: {
                    bail("unreachable");
                }
            }
            break;
        }
        default: {
            bail("unreachable");
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

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 1: {  // RRC r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (carry << 7);
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 2: {  // RL r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool prev_carry = (cpu->f & CPU_FLAG_C) != 0;
                    const bool new_carry = (value & 0x80) != 0;
                    const uint8_t new_value = (value << 1) | prev_carry;
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, new_carry);
                    break;
                }
                case 3: {  // RR r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool prev_carry = (cpu->f & CPU_FLAG_C) != 0;
                    const bool new_carry = value & 1;
                    const uint8_t new_value = (value >> 1) | (prev_carry << 7);
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, new_carry);
                    break;
                }
                case 4: {  // SLA r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = (value & 0x80) != 0;
                    const uint8_t new_value = value << 1;
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 5: {  // SRA r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t b7 = value & 0x80;
                    const uint8_t new_value = (value >> 1) | b7;
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, carry);
                    break;
                }
                case 6: {  // SWAP r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const uint8_t new_value = concat_u16(value & 0xFF, value >> 8);
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, false);
                    break;
                }
                case 7: {  // SRL r8
                    const uint8_t value = Cpu_read_r(cpu, mem, z);
                    const bool carry = value & 1;
                    const uint8_t new_value = value >> 1;
                    Cpu_write_r(cpu, mem, z, new_value);

                    Cpu_set_flag(cpu, CPU_FLAG_Z, new_value == 0);
                    Cpu_set_flag(cpu, CPU_FLAG_N, false);
                    Cpu_set_flag(cpu, CPU_FLAG_H, false);
                    Cpu_set_flag(cpu, CPU_FLAG_C, carry);
                    break;
                }
                default: {
                    bail("unreachable");
                }
            }
            break;
        }
        case 1: {  // BIT u3, r8
            const uint8_t value = Cpu_read_r(cpu, mem, z);
            Cpu_set_flag(cpu, CPU_FLAG_Z, (value & (1 << y)) == 0);
            Cpu_set_flag(cpu, CPU_FLAG_N, true);
            Cpu_set_flag(cpu, CPU_FLAG_H, false);
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
            bail("unreachable");
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
