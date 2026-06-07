#include "cpu.h"
#include "log.h"
#include "macros.h"
#include "num.h"
#include "stdinc.h"
#include <assert.h>

typedef enum : u8 {
    CPU_FLAG_C = 1 << 4,
    CPU_FLAG_H = 1 << 5,
    CPU_FLAG_N = 1 << 6,
    CPU_FLAG_Z = 1 << 7,
} CpuFlag;

typedef enum : u8 {
    R_B = 0,
    R_C = 1,
    R_D = 2,
    R_E = 3,
    R_H = 4,
    R_L = 5,
    R_HL = 6,
    R_A = 7,
} CpuTableR;

typedef enum : u8 {
    RP_BC = 0,
    RP_DE = 1,
    RP_HL = 2,
    RP_SP = 3,
} CpuTableRp;

typedef enum : u8 {
    RP2_BC = 0,
    RP2_DE = 1,
    RP2_HL = 2,
    RP2_AF = 3,
} CpuTableRp2;

typedef enum : u8 {
    CC_NZ = 0,
    CC_Z = 1,
    CC_NC = 2,
    CC_C = 3,
} CpuTableCc;

typedef enum : u8 {
    ALU_ADD = 0,
    ALU_ADC = 1,
    ALU_SUB = 2,
    ALU_SBC = 3,
    ALU_AND = 4,
    ALU_XOR = 5,
    ALU_OR = 6,
    ALU_CP = 7,
} CpuTableAlu;

Cpu cpu_init()
{
    return (Cpu){
        .mcycle_cnt = 0,
        .sp = 0,
        .pc = 0,
        .mode = MODE_RUNNING,
        .b = 0,
        .c = 0,
        .d = 0,
        .e = 0,
        .h = 0,
        .l = 0,
        .a = 0,
        .f = 0,
        .queued_ime = false,
        .ime = true,
    };
}

static bool cpu_read_cc(const Cpu *cpu, CpuTableCc cc)
{
    switch (cc) {
        case CC_NZ:
            return (cpu->f & CPU_FLAG_Z) == 0;
        case CC_Z:
            return (cpu->f & CPU_FLAG_Z) != 0;
        case CC_NC:
            return (cpu->f & CPU_FLAG_C) == 0;
        case CC_C:
            return (cpu->f & CPU_FLAG_C) != 0;
    }

    unreachable();
}

static u16 cpu_read_rp(const Cpu *cpu, CpuTableRp rp)
{
    switch (rp) {
        case RP_BC:
            return concat_u16(cpu->b, cpu->c);
        case RP_DE:
            return concat_u16(cpu->d, cpu->e);
        case RP_HL:
            return concat_u16(cpu->h, cpu->l);
        case RP_SP:
            return cpu->sp;
    }

    unreachable();
}

static void cpu_write_rp(Cpu *cpu, CpuTableRp rp, u16 value)
{
    switch (rp) {
        case RP_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            return;
        case RP_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            return;
        case RP_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            return;
        case RP_SP:
            cpu->sp = value;
            return;
    }

    unreachable();
}

static u16 cpu_read_rp2(const Cpu *cpu, CpuTableRp rp)
{
    switch (rp) {
        case RP2_BC:
            return concat_u16(cpu->b, cpu->c);
        case RP2_DE:
            return concat_u16(cpu->d, cpu->e);
        case RP2_HL:
            return concat_u16(cpu->h, cpu->l);
        case RP2_AF:
            return concat_u16(cpu->a, cpu->f);
    }

    unreachable();
}

void cpu_write_rp2(Cpu *cpu, CpuTableRp rp, u16 value)
{
    switch (rp) {
        case RP2_BC:
            cpu->b = value >> 8;
            cpu->c = value & 0xFF;
            return;
        case RP2_DE:
            cpu->d = value >> 8;
            cpu->e = value & 0xFF;
            return;
        case RP2_HL:
            cpu->h = value >> 8;
            cpu->l = value & 0xFF;
            return;
        case RP2_AF:
            cpu->a = value >> 8;
            cpu->f = value & 0xF0;
            return;
    }

    unreachable();
}

static u8 cpu_read_mem(Cpu *cpu, const Memory *mem, u16 addr)
{
    cpu->mcycle_cnt++;
    return mem->read(mem->ctx, addr);
}

static u16 cpu_read_mem_u16(Cpu *cpu, const Memory *mem, u16 addr)
{
    u8 lo = cpu_read_mem(cpu, mem, addr);
    u8 hi = cpu_read_mem(cpu, mem, addr + 1);
    return concat_u16(hi, lo);
}

static void cpu_write_mem(Cpu *cpu, Memory *mem, u16 addr, u8 value)
{
    cpu->mcycle_cnt++;
    mem->write(mem->ctx, addr, value);
}

static void cpu_write_mem_u16(Cpu *cpu, Memory *mem, u16 addr, u16 value)
{
    cpu_write_mem(cpu, mem, addr, value & 0xFF);
    cpu_write_mem(cpu, mem, addr + 1, value >> 8);
}

static u8 cpu_read_pc(Cpu *cpu, const Memory *mem)
{
    u8 value = cpu_read_mem(cpu, mem, cpu->pc);
    cpu->pc++;
    return value;
}

static u16 cpu_read_pc_u16(Cpu *cpu, const Memory *mem)
{
    u16 value = cpu_read_mem_u16(cpu, mem, cpu->pc);
    cpu->pc += 2;
    return value;
}

static void cpu_stack_push_u16(Cpu *cpu, Memory *mem, u16 value)
{
    cpu->sp -= 2;
    cpu_write_mem_u16(cpu, mem, cpu->sp, value);
    cpu->mcycle_cnt++;
}

static u16 cpu_stack_pop_u16(Cpu *cpu, const Memory *mem)
{
    u16 value = cpu_read_mem_u16(cpu, mem, cpu->sp);
    cpu->sp += 2;
    return value;
}

static u8 cpu_read_r(Cpu *cpu, const Memory *mem, CpuTableR r)
{
    switch (r) {
        case R_B:
            return cpu->b;
        case R_C:
            return cpu->c;
        case R_D:
            return cpu->d;
        case R_E:
            return cpu->e;
        case R_H:
            return cpu->h;
        case R_L:
            return cpu->l;
        case R_HL: {
            u16 addr = cpu_read_rp(cpu, RP_HL);
            return cpu_read_mem(cpu, mem, addr);
        }
        case R_A:
            return cpu->a;
    }

    unreachable();
}

static void cpu_write_r(Cpu *cpu, Memory *mem, CpuTableR r, u8 value)
{
    switch (r) {
        case R_B:
            cpu->b = value;
            return;
        case R_C:
            cpu->c = value;
            return;
        case R_D:
            cpu->d = value;
            return;
        case R_E:
            cpu->e = value;
            return;
        case R_H:
            cpu->h = value;
            return;
        case R_L:
            cpu->l = value;
            return;
        case R_HL: {
            u16 addr = cpu_read_rp(cpu, RP_HL);
            cpu_write_mem(cpu, mem, addr, value);
            return;
        }
        case R_A:
            cpu->a = value;
            return;
    }

    unreachable();
}

static inline void cpu_instr_add_u8(Cpu *cpu, u8 rhs)
{
    u8 prev_a = cpu->a;
    cpu->a += rhs;

    set_bits(&cpu->f, CPU_FLAG_C, rhs > (0xFF - prev_a));
    set_bits(&cpu->f, CPU_FLAG_H, (prev_a & 0xF) + (rhs & 0xF) > 0xF);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
}

static inline void cpu_instr_adc_u8(Cpu *cpu, u8 rhs)
{
    u8 prev_a = cpu->a;
    u8 carry = (cpu->f & CPU_FLAG_C) != 0;
    u16 result = (u16)prev_a + rhs + carry;
    cpu->a = (u8)result;

    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (prev_a & 0xF) + (rhs & 0xF) + carry > 0xF);
    set_bits(&cpu->f, CPU_FLAG_C, result > 0xFF);
}

static inline void cpu_instr_sub_u8(Cpu *cpu, u8 rhs)
{
    u8 prev_a = cpu->a;
    cpu->a -= rhs;

    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, (prev_a & 0xF) < (rhs & 0xF));
    set_bits(&cpu->f, CPU_FLAG_C, rhs > prev_a);
}

static inline void cpu_instr_sbc_u8(Cpu *cpu, u8 rhs)
{
    u8 prev_a = cpu->a;
    u8 borrow = (cpu->f & CPU_FLAG_C) != 0;
    cpu->a = prev_a - rhs - borrow;

    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, (prev_a & 0xF) < (rhs & 0xF) + borrow);
    set_bits(&cpu->f, CPU_FLAG_C, prev_a < (u16)rhs + borrow);
}

static inline void cpu_instr_and_u8(Cpu *cpu, u8 rhs)
{
    cpu->a &= rhs;
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, true);
    set_bits(&cpu->f, CPU_FLAG_C, false);
}

static inline void cpu_instr_xor_u8(Cpu *cpu, u8 rhs)
{
    cpu->a ^= rhs;
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, false);
}

static inline void cpu_instr_or_u8(Cpu *cpu, u8 rhs)
{
    cpu->a |= rhs;
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, false);
}

static inline void cpu_instr_cp_u8(Cpu *cpu, u8 rhs)
{
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == rhs);
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, (cpu->a & 0xF) < (rhs & 0xF));
    set_bits(&cpu->f, CPU_FLAG_C, cpu->a < rhs);
}

static inline void cpu_instr_alu(Cpu *cpu, CpuTableAlu alu, u8 rhs)
{
    // clang-format off
    switch (alu) {
        case ALU_ADD: cpu_instr_add_u8(cpu, rhs); break;
        case ALU_ADC: cpu_instr_adc_u8(cpu, rhs); break;
        case ALU_SUB: cpu_instr_sub_u8(cpu, rhs); break;
        case ALU_SBC: cpu_instr_sbc_u8(cpu, rhs); break;
        case ALU_AND: cpu_instr_and_u8(cpu, rhs); break;
        case ALU_XOR: cpu_instr_xor_u8(cpu, rhs); break;
        case ALU_OR: cpu_instr_or_u8(cpu, rhs); break;
        case ALU_CP: cpu_instr_cp_u8(cpu, rhs); break;
        default: BAIL("invalid alu: %i", alu);
    }
    // clang-format on
}

static inline void cpu_instr_nop()
{
    log_trace("nop");
}

static inline void cpu_instr_ld_n16_sp(Cpu *cpu, Memory *mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("ld [$%04X], SP", addr);

    cpu_write_mem_u16(cpu, mem, addr, cpu->sp);
}

static inline void cpu_instr_stop(Cpu *cpu)
{
    log_trace("stop");
    cpu->mode = MODE_STOPPED;

    log_debug("TODO: implement STOP instruction properly");
}

static inline void cpu_instr_jr_e8(Cpu *cpu, const Memory *mem)
{
    i8 offset = (i8)cpu_read_pc(cpu, mem);
    log_trace("jr %i", offset);

    cpu->pc += offset;
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_jr_cc_e8(Cpu *cpu, const Memory *mem, u8 y)
{
    u8 cc = y - 4;
    i8 offset = (i8)cpu_read_pc(cpu, mem);
    log_trace("jr cc(%i), %i", cc, offset);

    if (cpu_read_cc(cpu, cc)) {
        cpu->pc += offset;
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_ld_r16_n16(Cpu *cpu, const Memory *mem, u8 p)
{
    u16 value = cpu_read_pc_u16(cpu, mem);
    log_trace("ld rp(%d), $%04X", p, value);

    cpu_write_rp(cpu, p, value);
}

static inline void cpu_instr_add_hl_r16(Cpu *cpu, u8 p)
{
    log_trace("add hl, rp(%d)", p);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    u16 rhs = cpu_read_rp(cpu, p);

    cpu_write_rp(cpu, RP_HL, hl + rhs);

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (hl & 0xFFF) + (rhs & 0xFFF) > 0xFFF);
    set_bits(&cpu->f, CPU_FLAG_C, rhs > 0xFFFF - hl);

    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ld_bc_a(Cpu *cpu, Memory *mem)
{
    log_trace("ld [bc], a");

    u16 bc = cpu_read_rp(cpu, RP_BC);
    cpu_write_mem(cpu, mem, bc, cpu->a);
}

static inline void cpu_instr_ld_de_a(Cpu *cpu, Memory *mem)
{
    log_trace("ld [de], a");

    u16 de = cpu_read_rp(cpu, RP_DE);
    cpu_write_mem(cpu, mem, de, cpu->a);
}

static inline void cpu_instr_ld_hli_a(Cpu *cpu, Memory *mem)
{
    log_trace("ld [hl+], a");

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu_write_mem(cpu, mem, hl, cpu->a);
    cpu_write_rp(cpu, RP_HL, hl + 1);
}

static inline void cpu_instr_ld_hld_a(Cpu *cpu, Memory *mem)
{
    log_trace("ld [hl-], a");

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu_write_mem(cpu, mem, hl, cpu->a);
    cpu_write_rp(cpu, RP_HL, hl - 1);
}

static inline void cpu_instr_ld_a_bc(Cpu *cpu, const Memory *mem)
{
    log_trace("ld a, [bc]");
    u16 bc = cpu_read_rp(cpu, RP_BC);
    cpu->a = cpu_read_mem(cpu, mem, bc);
}

static inline void cpu_instr_ld_a_de(Cpu *cpu, const Memory *mem)
{
    log_trace("ld a, [de]");

    u16 de = cpu_read_rp(cpu, RP_DE);
    cpu->a = cpu_read_mem(cpu, mem, de);
}

static inline void cpu_instr_ld_a_hli(Cpu *cpu, const Memory *mem)
{
    log_trace("ld a, [hl+]");

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu->a = cpu_read_mem(cpu, mem, hl);
    cpu_write_rp(cpu, RP_HL, hl + 1);
}

static inline void cpu_instr_ld_a_hld(Cpu *cpu, const Memory *mem)
{
    log_trace("ld a, [hl-]");

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu->a = cpu_read_mem(cpu, mem, hl);
    cpu_write_rp(cpu, RP_HL, hl - 1);
}

static inline void cpu_instr_inc_r16(Cpu *cpu, u8 p)
{
    log_trace("inc rp(%d)", p);

    u16 value = cpu_read_rp(cpu, p);
    cpu_write_rp(cpu, p, value + 1);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_dec_r16(Cpu *cpu, u8 p)
{
    log_trace("dec rp(%d)", p);

    u16 value = cpu_read_rp(cpu, p);
    cpu_write_rp(cpu, p, value - 1);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_inc_r8(Cpu *cpu, Memory *mem, u8 y)
{
    log_trace("inc r(%d)", y);

    u8 value = cpu_read_r(cpu, mem, y);
    u8 new_value = value + 1;
    cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (new_value & 0xF) == 0);
}

static inline void cpu_instr_dec_r8(Cpu *cpu, Memory *mem, u8 y)
{
    log_trace("dec r(%d)", y);

    u8 value = cpu_read_r(cpu, mem, y);
    u8 new_value = value - 1;
    cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, (new_value & 0xF) == 0xF);
}

static inline void cpu_instr_ld_r8_n(Cpu *cpu, Memory *mem, u8 y)
{
    u8 value = cpu_read_pc(cpu, mem);
    log_trace("ld r(%d), $%02X", y, value);

    cpu_write_r(cpu, mem, y, value);
}

static inline void cpu_instr_rlca(Cpu *cpu)
{
    log_trace("rlca");

    u8 bit_7 = (cpu->a & 0x80) != 0;
    cpu->a = (cpu->a << 1) | bit_7;

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_rrca(Cpu *cpu)
{
    log_trace("rrca");

    u8 bit_0 = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (bit_0 << 7);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_rla(Cpu *cpu)
{
    log_trace("rla");

    u8 prev_carry = (cpu->f & CPU_FLAG_C) != 0;
    u8 new_carry = (cpu->a & 0x80) != 0;
    cpu->a = (cpu->a << 1) | prev_carry;

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, new_carry);
}

static inline void cpu_instr_rra(Cpu *cpu)
{
    log_trace("rra");

    u8 prev_carry = (cpu->f & CPU_FLAG_C) != 0;
    u8 new_carry = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (prev_carry << 7);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, new_carry);
}

static inline void cpu_instr_daa(Cpu *cpu)
{
    log_trace("daa");

    u8 adj = 0;

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
            set_bits(&cpu->f, CPU_FLAG_C, true);
        }

        cpu->a += adj;
    }

    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_Z, cpu->a == 0);
}

static inline void cpu_instr_cpl(Cpu *cpu)
{
    log_trace("cpl");

    cpu->a = ~cpu->a;
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, true);
}

static inline void cpu_instr_scf(Cpu *cpu)
{
    log_trace("scf");

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, true);
}

static inline void cpu_instr_ccf(Cpu *cpu)
{
    log_trace("ccf");

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, !(cpu->f & CPU_FLAG_C));
}

static inline void cpu_instr_halt(Cpu *cpu)
{
    log_trace("halt");
    cpu->mode = MODE_HALTED;
}

static inline void cpu_instr_ld_r8_r8(Cpu *cpu, Memory *mem, u8 y, u8 z)
{
    u8 value = cpu_read_r(cpu, mem, z);
    log_trace("ld r(%d), r(%d)", y, z);

    cpu_write_r(cpu, mem, y, value);
}

static inline void cpu_instr_alu_r8(Cpu *cpu, Memory *mem, u8 y, u8 z)
{

    log_trace("{alu} a, r(%d)", z);

    u8 rhs = cpu_read_r(cpu, mem, z);
    cpu_instr_alu(cpu, y, rhs);
}

static inline void cpu_instr_ldh_n16_a(Cpu *cpu, Memory *mem)
{
    u8 offset = cpu_read_pc(cpu, mem);
    log_trace("ldh [$%02X], a", offset);

    u16 addr = 0xFF00 + offset;
    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_add_sp_e8(Cpu *cpu, Memory *mem)
{
    u8 offset_u8 = (i8)cpu_read_pc(cpu, mem);
    i8 offset = (i8)offset_u8;
    log_trace("add sp, %d", offset);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CPU_FLAG_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    cpu->sp += offset;
    cpu->mcycle_cnt += 2;
}

static inline void cpu_instr_ldh_a_n16(Cpu *cpu, const Memory *mem)
{
    u8 offset = cpu_read_pc(cpu, mem);
    log_trace("ldh a, [$%02X]", offset);

    u16 addr = 0xFF00 + offset;
    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_ld_hl_sp_plus_e8(Cpu *cpu, const Memory *mem)
{
    u8 offset_u8 = (i8)cpu_read_pc(cpu, mem);
    i8 offset = (i8)offset_u8;
    log_trace("ld hl, sp%+d", offset);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CPU_FLAG_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    cpu_write_rp(cpu, RP_HL, cpu->sp + offset);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ret_cc(Cpu *cpu, Memory *mem, u8 y)
{
    log_trace("ret cc(%d)", y);

    cpu->mcycle_cnt++;
    if (cpu_read_cc(cpu, y)) {
        cpu->pc = cpu_stack_pop_u16(cpu, mem);
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_pop_r16(Cpu *cpu, Memory *mem, u8 p)
{
    log_trace("pop rp2(%d)", p);

    u16 value = cpu_stack_pop_u16(cpu, mem);
    cpu_write_rp2(cpu, p, value);
}

static inline void cpu_instr_ret(Cpu *cpu, Memory *mem)
{
    log_trace("ret");

    cpu->pc = cpu_stack_pop_u16(cpu, mem);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_reti(Cpu *cpu, Memory *mem)
{
    log_trace("reti");

    cpu->ime = true;
    cpu->pc = cpu_stack_pop_u16(cpu, mem);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_jp_hl(Cpu *cpu)
{
    log_trace("jp hl");

    cpu->pc = cpu_read_rp(cpu, RP_HL);
}

static inline void cpu_instr_ld_sp_hl(Cpu *cpu)
{
    log_trace("ld sp, hl");

    cpu->sp = cpu_read_rp(cpu, RP_HL);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ldh_c_a(Cpu *cpu, Memory *mem)
{
    log_trace("ldh [c], a");

    u16 addr = 0xFF00 + cpu->c;
    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_ld_a16_a(Cpu *cpu, Memory *mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("ld [$%04X], a", addr);

    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_ldh_a_c(Cpu *cpu, const Memory *mem)
{
    u16 addr = 0xFF00 + cpu->c;
    log_trace("ld a, [c]");

    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_ld_a_a16(Cpu *cpu, const Memory *mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("ld a, [$%04X]", addr);

    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_jp_cc_a16(Cpu *cpu, const Memory *mem, u8 y)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("jp cc(%d), $%04X", y, addr);

    if (cpu_read_cc(cpu, y)) {
        cpu->pc = addr;
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_jp_a16(Cpu *cpu, const Memory *mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("jp $%04X", addr);

    cpu->pc = addr;
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_di(Cpu *cpu)
{
    log_trace("di");

    cpu->ime = false;
    cpu->queued_ime = false;
}

static inline void cpu_instr_ei(Cpu *cpu)
{
    log_trace("ei");

    cpu->queued_ime = true;
}

static inline void cpu_instr_call_cc_n16(Cpu *cpu, Memory *mem, u8 y)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("call cc(%d), $%04X", y, addr);

    if (cpu_read_cc(cpu, y)) {
        cpu_stack_push_u16(cpu, mem, cpu->pc);
        cpu->pc = addr;
    }
}

static inline void cpu_instr_push_r16(Cpu *cpu, Memory *mem, u8 p)
{
    log_trace("push rp2(%d)", p);

    u16 value = cpu_read_rp2(cpu, p);
    cpu_stack_push_u16(cpu, mem, value);
}

static inline void cpu_instr_call_n16(Cpu *cpu, Memory *mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    log_trace("call $%04X", addr);

    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = addr;
}

static inline void cpu_instr_alu_a_a8(Cpu *cpu, Memory *mem, u8 y)
{
    u8 rhs = cpu_read_pc(cpu, mem);
    log_trace("{alu} a, $%02X", rhs);

    cpu_instr_alu(cpu, y, rhs);
}

static inline void cpu_instr_rst_vec(Cpu *cpu, Memory *mem, u8 y)
{
    log_trace("rst $%02X", y * 8);

    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = y << 3;
}

static inline void cpu_instr_rlc_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("rlc r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_7 = (value & 0x80) != 0;
    u8 new_value = (value << 1) | bit_7;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_rrc_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("rrc r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_0 = value & 1;
    u8 new_value = (value >> 1) | (bit_0 << 7);
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_rl_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("rl r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 prev_carry = (cpu->f & CPU_FLAG_C) != 0;
    u8 new_carry = (value & 0x80) != 0;

    u8 new_value = (value << 1) | prev_carry;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, new_carry);
}

static inline void cpu_instr_rr_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("rr r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 prev_carry = (cpu->f & CPU_FLAG_C) != 0;
    u8 new_carry = value & 1;

    u8 new_value = (value >> 1) | (prev_carry << 7);
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, new_carry);
}

static inline void cpu_instr_sla_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("sla r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_7 = (value & 0x80) != 0;
    u8 new_value = value << 1;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_sra_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("sra r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_0 = value & 1;
    u8 bit_7 = (value & 0x80) != 0;
    u8 new_value = (value >> 1) | (bit_7 << 7);
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_swap_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("swap r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 prev_hi = value >> 4;
    u8 prev_lo = value & 0xF;
    u8 new_value = (prev_lo << 4) | prev_hi;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, false);
}

static inline void cpu_instr_srl_r8(Cpu *cpu, Memory *mem, u8 z)
{
    log_trace("srl r(%d)", z);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_0 = value & 1;
    u8 new_value = value >> 1;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_bit_u3_r8(Cpu *cpu, Memory *mem, u8 y, u8 z)
{
    log_trace("bit %d,r(%d)", y, z);

    u8 value = cpu_read_r(cpu, mem, z);
    set_bits(&cpu->f, CPU_FLAG_Z, (value & (1 << y)) == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, true);
}

static inline void cpu_instr_res_u3_r8(Cpu *cpu, Memory *mem, u8 y, u8 z)
{
    log_trace("res %d,r(%d)", y, z);

    u8 value = cpu_read_r(cpu, mem, z);
    cpu_write_r(cpu, mem, z, value & ~(1 << y));
}

static inline void cpu_instr_set_u3_r8(Cpu *cpu, Memory *mem, u8 y, u8 z)
{
    log_trace("set %d,r(%d)", y, z);

    u8 value = cpu_read_r(cpu, mem, z);
    cpu_write_r(cpu, mem, z, value | (1 << y));
}

static inline void cpu_instr_prefix(Cpu *cpu, Memory *mem)
{
    u8 opcode = cpu_read_pc(cpu, mem);
    log_trace("{prefix} $%02X", opcode);
    log_trace("    prefixed (opcode = $%02X)", opcode);

    u8 x = opcode >> 6;
    u8 y = (opcode >> 3) & 0b111;
    u8 z = opcode & 0b111;

    // clang-format off
    switch (x) {
        case 0:
            switch (y) {
                case 0: cpu_instr_rlc_r8(cpu, mem, z); break;
                case 1: cpu_instr_rrc_r8(cpu, mem, z); break;
                case 2: cpu_instr_rl_r8(cpu, mem, z); break;
                case 3: cpu_instr_rr_r8(cpu, mem, z); break;
                case 4: cpu_instr_sla_r8(cpu, mem, z); break;
                case 5: cpu_instr_sra_r8(cpu, mem, z); break;
                case 6: cpu_instr_swap_r8(cpu, mem, z); break;
                case 7: cpu_instr_srl_r8(cpu, mem, z); break;
                default: unreachable();
            }
            break;
        case 1: cpu_instr_bit_u3_r8(cpu, mem, y, z); break;
        case 2: cpu_instr_res_u3_r8(cpu, mem, y, z); break;
        case 3: cpu_instr_set_u3_r8(cpu, mem, y, z); break;
        default: unreachable();
    }
    // clang-format on
}

// NOLINTNEXTLINE
void cpu_execute(Cpu *cpu, Memory *mem, u8 opcode)
{
    // Credit:
    // https://archive.gbdev.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html

    u8 x = opcode >> 6;
    u8 y = (opcode >> 3) & 0b111;
    u8 z = opcode & 0b111;
    u8 p = y >> 1;
    u8 q = y & 1;

    switch (x) {
        case 0:
            switch (z) {
                case 0:
                    // clang-format off
                    switch (y) {
                        case 0: cpu_instr_nop(); break;
                        case 1: cpu_instr_ld_n16_sp(cpu, mem); break;
                        case 2: cpu_instr_stop(cpu); break;
                        case 3: cpu_instr_jr_e8(cpu, mem); break;
                        default: cpu_instr_jr_cc_e8(cpu, mem, y);
                    }
                    // clang-format on
                    break;
                case 1:
                    if (q == 0)
                        cpu_instr_ld_r16_n16(cpu, mem, p);
                    else
                        cpu_instr_add_hl_r16(cpu, p);
                    break;
                case 2:
                    if (q == 0) {
                        // clang-format off
                        switch (p) {
                            case 0: cpu_instr_ld_bc_a(cpu, mem); break;
                            case 1: cpu_instr_ld_de_a(cpu, mem); break;
                            case 2: cpu_instr_ld_hli_a(cpu, mem); break;
                            case 3: cpu_instr_ld_hld_a(cpu, mem); break;
                            default: unreachable();
                        }
                        // clang-format on
                    } else {
                        // clang-format off
                        switch (p) {
                            case 0: cpu_instr_ld_a_bc(cpu, mem); break;
                            case 1: cpu_instr_ld_a_de(cpu, mem); break;
                            case 2: cpu_instr_ld_a_hli(cpu, mem); break;
                            case 3: cpu_instr_ld_a_hld(cpu, mem); break;
                            default: unreachable();
                        }
                        // clang-format on
                    }
                    break;
                case 3:
                    if (q == 0)
                        cpu_instr_inc_r16(cpu, p);
                    else
                        cpu_instr_dec_r16(cpu, p);
                    break;
                    // clang-format off
                case 4: cpu_instr_inc_r8(cpu, mem, y); break;
                case 5: cpu_instr_dec_r8(cpu, mem, y); break;
                case 6: cpu_instr_ld_r8_n(cpu, mem, y); break;
                    // clang-format on
                case 7:
                    // clang-format off
                    switch (y) {
                        case 0: cpu_instr_rlca(cpu); break;
                        case 1: cpu_instr_rrca(cpu); break;
                        case 2: cpu_instr_rla(cpu); break;
                        case 3: cpu_instr_rra(cpu); break;
                        case 4: cpu_instr_daa(cpu); break;
                        case 5: cpu_instr_cpl(cpu); break;
                        case 6: cpu_instr_scf(cpu); break;
                        case 7: cpu_instr_ccf(cpu); break;
                        default: unreachable();
                    }
                    // clang-format on
                    break;
                default:
                    unreachable();
            }
            break;
        case 1:
            if (z == 6 && y == 6)
                cpu_instr_halt(cpu);
            else
                cpu_instr_ld_r8_r8(cpu, mem, y, z);
            break;
        case 2:
            cpu_instr_alu_r8(cpu, mem, y, z);
            break;
        case 3:
            switch (z) {
                case 0:
                    // clang-format off
                    switch (y) {
                        case 4: cpu_instr_ldh_n16_a(cpu, mem); break;
                        case 5: cpu_instr_add_sp_e8(cpu, mem); break;
                        case 6: cpu_instr_ldh_a_n16(cpu, mem); break;
                        case 7: cpu_instr_ld_hl_sp_plus_e8(cpu, mem); break;
                        default: cpu_instr_ret_cc(cpu, mem, y);
                    }
                    // clang-format on
                    break;
                case 1:
                    if (q == 0)
                        cpu_instr_pop_r16(cpu, mem, p);
                    else {
                        // clang-format off
                        switch (p) {
                            case 0: cpu_instr_ret(cpu, mem); break;
                            case 1: cpu_instr_reti(cpu, mem); break;
                            case 2: cpu_instr_jp_hl(cpu); break;
                            case 3: cpu_instr_ld_sp_hl(cpu); break;
                            default: unreachable();
                        }
                        // clang-format on
                    }
                    break;
                case 2:
                    // clang-format off
                    switch (y) {
                        case 4: cpu_instr_ldh_c_a(cpu, mem); break;
                        case 5: cpu_instr_ld_a16_a(cpu, mem); break;
                        case 6: cpu_instr_ldh_a_c(cpu, mem); break;
                        case 7: cpu_instr_ld_a_a16(cpu, mem); break;
                        default: cpu_instr_jp_cc_a16(cpu, mem, y);
                    }
                    // clang-format on
                    break;
                case 3:
                    // clang-format off
                    switch (y) {
                        case 0: cpu_instr_jp_a16(cpu, mem); break;
                        case 1: cpu_instr_prefix(cpu, mem); break;
                        case 6: cpu_instr_di(cpu); break;
                        case 7: cpu_instr_ei(cpu); break;
                        default: BAIL("removed instruction");
                    }
                    // clang-format on
                    break;
                case 4:
                    if (y < 4)
                        cpu_instr_call_cc_n16(cpu, mem, y);
                    else
                        BAIL("removed instruction");
                    break;
                case 5:
                    if (q == 0)
                        cpu_instr_push_r16(cpu, mem, p);
                    else if (p == 0)
                        cpu_instr_call_n16(cpu, mem);
                    else
                        BAIL("removed instruction");
                    break;
                case 6:
                    cpu_instr_alu_a_a8(cpu, mem, y);
                    break;
                case 7:
                    cpu_instr_rst_vec(cpu, mem, y);
                    break;
                default:
                    unreachable();
            }
            break;
        default:
            unreachable();
    }
}

void cpu_step(Cpu *cpu, Memory *mem)
{
    if (cpu->mode != MODE_RUNNING) {
        ++cpu->mcycle_cnt; // Makes the frontend work lmao
        assert(false && "check this out");
        return;
    }

    if (cpu->queued_ime) {
        cpu->ime = true;
        cpu->queued_ime = false;
    }

    u8 opcode = cpu_read_pc(cpu, mem);
    cpu_execute(cpu, mem, opcode);
}

void cpu_interrupt(Cpu *cpu, Memory *mem, u8 handler_location)
{
    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->ime = false;
    cpu->pc = handler_location;
    cpu->mcycle_cnt += 2;
}
