#include "cpu.h"
#include "../lib/log.h"
#include "../lib/num.h"

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
    };
}

void Cpu_set_flag(Cpu* const restrict cpu, const CpuFlag flag, const bool value) {
    if (value) {
        cpu->f |= flag;
    } else {
        cpu->f &= ~flag;
    }
}

bool Cpu_read_cc(const Cpu* restrict cpu, const CpuTableCc cc) {
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
            BAIL("invalid cc: %i", cc);
    }
}

uint16_t Cpu_read_rp(const Cpu* restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CPU_RP_BC:
            return CONCAT_U16(cpu->b, cpu->c);
        case CPU_RP_DE:
            return CONCAT_U16(cpu->d, cpu->e);
        case CPU_RP_HL:
            return CONCAT_U16(cpu->h, cpu->l);
        case CPU_RP_SP:
            return cpu->sp;
        default:
            BAIL("invalid rp: %i", rp);
    }
}

void Cpu_write_rp(Cpu* restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CPU_RP_BC:
            cpu->b = HI(value);
            cpu->c = LO(value);
            break;
        case CPU_RP_DE:
            cpu->d = HI(value);
            cpu->e = LO(value);
            break;
        case CPU_RP_HL:
            cpu->h = HI(value);
            cpu->l = LO(value);
            break;
        case CPU_RP_SP:
            cpu->sp = value;
            break;
        default:
            BAIL("invalid rp: %i", rp);
    }
}

uint16_t Cpu_read_rp2(const Cpu* restrict cpu, const CpuTableRp rp) {
    switch (rp) {
        case CPU_RP2_BC:
            return CONCAT_U16(cpu->b, cpu->c);
        case CPU_RP2_DE:
            return CONCAT_U16(cpu->d, cpu->e);
        case CPU_RP2_HL:
            return CONCAT_U16(cpu->h, cpu->l);
        case CPU_RP2_AF:
            return CONCAT_U16(cpu->a, cpu->f);
        default:
            BAIL("invalid rp2: %i", rp);
    }
}

void Cpu_write_rp2(Cpu* restrict cpu, const CpuTableRp rp, const uint16_t value) {
    switch (rp) {
        case CPU_RP2_BC:
            cpu->b = HI(value);
            cpu->c = LO(value);
            break;
        case CPU_RP2_DE:
            cpu->d = HI(value);
            cpu->e = LO(value);
            break;
        case CPU_RP2_HL:
            cpu->h = HI(value);
            cpu->l = LO(value);
            break;
        case CPU_RP2_AF:
            cpu->a = HI(value);
            cpu->f = LO(value);
            break;
        default:
            BAIL("invalid rp2: %i", rp);
    }
}

void Cpu_alu(Cpu* restrict cpu, const CpuTableAlu alu, const uint8_t rhs) {
    switch (alu) {
        case CPU_ALU_ADD: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > (0xFF - cpu->a));
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) + (rhs & 0xF)) & 0x10) != 0);
            cpu->a += rhs;
            Cpu_set_flag(cpu, CPU_FLAG_N, false);
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_ADC: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > (0xFF - cpu->a));
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) + (rhs & 0xF)) & 0x10) != 0);
            cpu->a += rhs;

            if (cpu->f & CPU_FLAG_C) {
                if (cpu->a == 0xFF) {
                    Cpu_set_flag(cpu, CPU_FLAG_C, true);
                }

                if ((cpu->a & 0xF) == 0xF) {
                    Cpu_set_flag(cpu, CPU_FLAG_H, true);
                }

                cpu->a++;
            }

            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            Cpu_set_flag(cpu, CPU_FLAG_N, false);
            break;
        }
        case CPU_ALU_SUB: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            cpu->a -= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_N, true);
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_SBC: {
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            cpu->a -= rhs;

            if (cpu->f & CPU_FLAG_C) {
                if (cpu->a == 0) {
                    Cpu_set_flag(cpu, CPU_FLAG_C, true);
                }

                if ((cpu->a & 0xF) == 0) {
                    Cpu_set_flag(cpu, CPU_FLAG_H, true);
                }

                cpu->a--;
            }

            Cpu_set_flag(cpu, CPU_FLAG_N, true);
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            break;
        }
        case CPU_ALU_AND: {
            cpu->a &= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            Cpu_set_flag(cpu, CPU_FLAG_N, false);
            Cpu_set_flag(cpu, CPU_FLAG_H, true);
            Cpu_set_flag(cpu, CPU_FLAG_C, false);
            break;
        }
        case CPU_ALU_XOR: {
            cpu->a ^= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            Cpu_set_flag(cpu, CPU_FLAG_N, false);
            Cpu_set_flag(cpu, CPU_FLAG_H, false);
            Cpu_set_flag(cpu, CPU_FLAG_C, false);
            break;
        }
        case CPU_ALU_OR: {
            cpu->a |= rhs;
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == 0);
            Cpu_set_flag(cpu, CPU_FLAG_N, false);
            Cpu_set_flag(cpu, CPU_FLAG_H, false);
            Cpu_set_flag(cpu, CPU_FLAG_C, false);
            break;
        }
        case CPU_ALU_CP: {
            Cpu_set_flag(cpu, CPU_FLAG_Z, cpu->a == rhs);
            Cpu_set_flag(cpu, CPU_FLAG_N, true);
            Cpu_set_flag(cpu, CPU_FLAG_H, (((cpu->a & 0xF) - (rhs & 0xF)) & 0x10) != 0);
            Cpu_set_flag(cpu, CPU_FLAG_C, rhs > cpu->a);
            break;
        }
        default: {
            BAIL("invalid alu: %i", alu);
        }
    }
}
