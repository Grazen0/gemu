#include "cpu.h"
#include "log.h"
#include "macros.h"
#include "util.h"
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

static const char *NAME_R[] = {
    [R_B] = "b", [R_C] = "c", [R_D] = "d",   [R_E] = "e",
    [R_H] = "h", [R_L] = "l", [R_HL] = "hl", [R_A] = "a",
};

typedef enum : u8 {
    RP_BC = 0,
    RP_DE = 1,
    RP_HL = 2,
    RP_SP = 3,
} CpuTableRp;

static const char *NAME_RP[] = {
    [RP_BC] = "bc",
    [RP_DE] = "de",
    [RP_HL] = "hl",
    [RP_SP] = "sp",
};

typedef enum : u8 {
    RP2_BC = 0,
    RP2_DE = 1,
    RP2_HL = 2,
    RP2_AF = 3,
} CpuTableRp2;

static const char *NAME_RP2[] = {
    [RP2_BC] = "bc",
    [RP2_DE] = "de",
    [RP2_HL] = "hl",
    [RP2_AF] = "af",
};

typedef enum : u8 {
    CC_NZ = 0,
    CC_Z = 1,
    CC_NC = 2,
    CC_C = 3,
} CpuTableCc;

static const char *NAME_CC[] = {
    [CC_NZ] = "nz",
    [CC_Z] = "z",
    [CC_NC] = "nc",
    [CC_C] = "c",
};

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

static const char *NAME_ALU[] = {
    [ALU_ADD] = "add", [ALU_ADC] = "adc", [ALU_SUB] = "sub", [ALU_SBC] = "sbc",
    [ALU_AND] = "and", [ALU_XOR] = "xor", [ALU_OR] = "or",   [ALU_CP] = "cp",
};

Cpu cpu_init(Sink sink)
{
    return (Cpu){
        .sink = sink,
        .mcycle_cnt = 0,
        .sp = 0,
        .pc = 0,
        .start_pc = 0,
        .mode = CPU_MODE_RUNNING,
        .b = 0,
        .c = 0,
        .d = 0,
        .e = 0,
        .h = 0,
        .l = 0,
        .a = 0,
        .f = 0,
        .queued_ime = false,
        .ime = false,
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

static u8 cpu_read_mem(Cpu *cpu, Memory mem, u16 addr)
{
    cpu->mcycle_cnt++;
    return mem_read(mem, addr);
}

static u16 cpu_read_mem_u16(Cpu *cpu, Memory mem, u16 addr)
{
    u8 lo = cpu_read_mem(cpu, mem, addr);
    u8 hi = cpu_read_mem(cpu, mem, addr + 1);
    return concat_u16(hi, lo);
}

static void cpu_write_mem(Cpu *cpu, Memory mem, u16 addr, u8 value)
{
    cpu->mcycle_cnt++;
    mem_write(mem, addr, value);
}

static void cpu_write_mem_u16(Cpu *cpu, Memory mem, u16 addr, u16 value)
{
    cpu_write_mem(cpu, mem, addr, value & 0xFF);
    cpu_write_mem(cpu, mem, addr + 1, value >> 8);
}

static u8 cpu_read_pc(Cpu *cpu, Memory mem)
{
    u8 value = cpu_read_mem(cpu, mem, cpu->pc);
    cpu->pc++;
    return value;
}

static u16 cpu_read_pc_u16(Cpu *cpu, Memory mem)
{
    u16 value = cpu_read_mem_u16(cpu, mem, cpu->pc);
    cpu->pc += 2;
    return value;
}

static void cpu_stack_push_u16(Cpu *cpu, Memory mem, u16 value)
{
    cpu->sp -= 2;
    cpu_write_mem_u16(cpu, mem, cpu->sp, value);
    cpu->mcycle_cnt++;
}

static u16 cpu_stack_pop_u16(Cpu *cpu, Memory mem)
{
    u16 value = cpu_read_mem_u16(cpu, mem, cpu->sp);
    cpu->sp += 2;
    return value;
}

static u8 cpu_read_r(Cpu *cpu, Memory mem, CpuTableR r)
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

static void cpu_write_r(Cpu *cpu, Memory mem, CpuTableR r, u8 value)
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

static inline void cpu_instr_nop(const Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   nop", cpu->start_pc);
}

static inline void cpu_instr_ld_n16_sp(Cpu *cpu, Memory mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   ld [$%04X], SP", cpu->start_pc, addr);

    cpu_write_mem_u16(cpu, mem, addr, cpu->sp);
}

static inline void cpu_instr_stop(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   stop", cpu->start_pc);
    cpu->mode = CPU_MODE_STOPPED;

    log_warn("TODO: implement STOP instruction properly");
}

static inline void cpu_instr_jr_e8(Cpu *cpu, Memory mem)
{
    i8 offset = (i8)cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   jr %i", cpu->start_pc, offset);

    cpu->pc += offset;
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_jr_cc_e8(Cpu *cpu, Memory mem, u8 y)
{
    u8 cc = y - 4;
    i8 offset = (i8)cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   jr %s, %i", cpu->start_pc, NAME_CC[cc], offset);

    if (cpu_read_cc(cpu, cc)) {
        cpu->pc += offset;
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_ld_r16_n16(Cpu *cpu, Memory mem, u8 p)
{
    u16 value = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   ld %s, $%04X", cpu->start_pc, NAME_RP[p],
             value);

    cpu_write_rp(cpu, p, value);
}

static inline void cpu_instr_add_hl_r16(Cpu *cpu, u8 p)
{
    sink_log(cpu->sink, "%04X   add hl, %s", cpu->start_pc, NAME_RP[p]);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    u16 rhs = cpu_read_rp(cpu, p);

    cpu_write_rp(cpu, RP_HL, hl + rhs);

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (hl & 0xFFF) + (rhs & 0xFFF) > 0xFFF);
    set_bits(&cpu->f, CPU_FLAG_C, rhs > 0xFFFF - hl);

    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ld_bc_a(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld [bc], a", cpu->start_pc);

    u16 bc = cpu_read_rp(cpu, RP_BC);
    cpu_write_mem(cpu, mem, bc, cpu->a);
}

static inline void cpu_instr_ld_de_a(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld [de], a", cpu->start_pc);

    u16 de = cpu_read_rp(cpu, RP_DE);
    cpu_write_mem(cpu, mem, de, cpu->a);
}

static inline void cpu_instr_ld_hli_a(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld [hl+], a", cpu->start_pc);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu_write_mem(cpu, mem, hl, cpu->a);
    cpu_write_rp(cpu, RP_HL, hl + 1);
}

static inline void cpu_instr_ld_hld_a(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld [hl-], a", cpu->start_pc);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu_write_mem(cpu, mem, hl, cpu->a);
    cpu_write_rp(cpu, RP_HL, hl - 1);
}

static inline void cpu_instr_ld_a_bc(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld a, [bc]", cpu->start_pc);
    u16 bc = cpu_read_rp(cpu, RP_BC);
    cpu->a = cpu_read_mem(cpu, mem, bc);
}

static inline void cpu_instr_ld_a_de(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld a, [de]", cpu->start_pc);

    u16 de = cpu_read_rp(cpu, RP_DE);
    cpu->a = cpu_read_mem(cpu, mem, de);
}

static inline void cpu_instr_ld_a_hli(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld a, [hl+]", cpu->start_pc);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu->a = cpu_read_mem(cpu, mem, hl);
    cpu_write_rp(cpu, RP_HL, hl + 1);
}

static inline void cpu_instr_ld_a_hld(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ld a, [hl-]", cpu->start_pc);

    u16 hl = cpu_read_rp(cpu, RP_HL);
    cpu->a = cpu_read_mem(cpu, mem, hl);
    cpu_write_rp(cpu, RP_HL, hl - 1);
}

static inline void cpu_instr_inc_r16(Cpu *cpu, u8 p)
{
    sink_log(cpu->sink, "%04X   inc %s", cpu->start_pc, NAME_RP[p]);

    u16 value = cpu_read_rp(cpu, p);
    cpu_write_rp(cpu, p, value + 1);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_dec_r16(Cpu *cpu, u8 p)
{
    sink_log(cpu->sink, "%04X   dec %s", cpu->start_pc, NAME_RP[p]);

    u16 value = cpu_read_rp(cpu, p);
    cpu_write_rp(cpu, p, value - 1);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_inc_r8(Cpu *cpu, Memory mem, u8 y)
{
    sink_log(cpu->sink, "%04X   inc %s", cpu->start_pc, NAME_R[y]);

    u8 value = cpu_read_r(cpu, mem, y);
    u8 new_value = value + 1;
    cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (new_value & 0xF) == 0);
}

static inline void cpu_instr_dec_r8(Cpu *cpu, Memory mem, u8 y)
{
    sink_log(cpu->sink, "%04X   dec %s", cpu->start_pc, NAME_R[y]);

    u8 value = cpu_read_r(cpu, mem, y);
    u8 new_value = value - 1;
    cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, (new_value & 0xF) == 0xF);
}

static inline void cpu_instr_ld_r8_n(Cpu *cpu, Memory mem, u8 y)
{
    u8 value = cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   ld %s, $%02X", cpu->start_pc, NAME_R[y], value);

    cpu_write_r(cpu, mem, y, value);
}

static inline void cpu_instr_rlca(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   rlca", cpu->start_pc);

    u8 bit_7 = (cpu->a & 0x80) != 0;
    cpu->a = (cpu->a << 1) | bit_7;

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_rrca(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   rrca", cpu->start_pc);

    u8 bit_0 = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (bit_0 << 7);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_rla(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   rla", cpu->start_pc);

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
    sink_log(cpu->sink, "%04X   rra", cpu->start_pc);

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
    sink_log(cpu->sink, "%04X   daa", cpu->start_pc);

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
    sink_log(cpu->sink, "%04X   cpl", cpu->start_pc);

    cpu->a = ~cpu->a;
    set_bits(&cpu->f, CPU_FLAG_N, true);
    set_bits(&cpu->f, CPU_FLAG_H, true);
}

static inline void cpu_instr_scf(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   scf", cpu->start_pc);

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, true);
}

static inline void cpu_instr_ccf(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   ccf", cpu->start_pc);

    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, !(cpu->f & CPU_FLAG_C));
}

static inline void cpu_instr_halt(Cpu *cpu)
{
    if (!cpu->ime)
        BAIL("TODO: implement HALT bug");

    sink_log(cpu->sink, "%04X   halt", cpu->start_pc);
    cpu->mode = CPU_MODE_HALTED;
}

static inline void cpu_instr_ld_r8_r8(Cpu *cpu, Memory mem, u8 y, u8 z)
{
    u8 value = cpu_read_r(cpu, mem, z);
    sink_log(cpu->sink, "%04X   ld %s, %s", cpu->start_pc, NAME_R[y],
             NAME_R[z]);

    cpu_write_r(cpu, mem, y, value);
}

static inline void cpu_instr_alu_r8(Cpu *cpu, Memory mem, u8 y, u8 z)
{

    sink_log(cpu->sink, "%04X   %s a, %s", cpu->start_pc, NAME_ALU[y],
             NAME_R[z]);

    u8 rhs = cpu_read_r(cpu, mem, z);
    cpu_instr_alu(cpu, y, rhs);
}

static inline void cpu_instr_ldh_n16_a(Cpu *cpu, Memory mem)
{
    u8 offset = cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   ldh [$%02X], a", cpu->start_pc, offset);

    u16 addr = 0xFF00 + offset;
    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_add_sp_e8(Cpu *cpu, Memory mem)
{
    u8 offset_u8 = (i8)cpu_read_pc(cpu, mem);
    i8 offset = (i8)offset_u8;
    sink_log(cpu->sink, "%04X   add sp, %d", cpu->start_pc, offset);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CPU_FLAG_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    cpu->sp += offset;
    cpu->mcycle_cnt += 2;
}

static inline void cpu_instr_ldh_a_n16(Cpu *cpu, Memory mem)
{
    u8 offset = cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   ldh a, [$%02X]", cpu->start_pc, offset);

    u16 addr = 0xFF00 + offset;
    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_ld_hl_sp_plus_e8(Cpu *cpu, Memory mem)
{
    u8 offset_u8 = (i8)cpu_read_pc(cpu, mem);
    i8 offset = (i8)offset_u8;
    sink_log(cpu->sink, "%04X   ld hl, sp%+d", cpu->start_pc, offset);

    set_bits(&cpu->f, CPU_FLAG_Z, false);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CPU_FLAG_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    cpu_write_rp(cpu, RP_HL, cpu->sp + offset);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ret_cc(Cpu *cpu, Memory mem, u8 y)
{
    sink_log(cpu->sink, "%04X   ret %s", cpu->start_pc, NAME_CC[y]);

    cpu->mcycle_cnt++;
    if (cpu_read_cc(cpu, y)) {
        cpu->pc = cpu_stack_pop_u16(cpu, mem);
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_pop_r16(Cpu *cpu, Memory mem, u8 p)
{
    sink_log(cpu->sink, "%04X   pop %s", cpu->start_pc, NAME_RP2[p]);

    u16 value = cpu_stack_pop_u16(cpu, mem);
    cpu_write_rp2(cpu, p, value);
}

static inline void cpu_instr_ret(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ret", cpu->start_pc);

    cpu->pc = cpu_stack_pop_u16(cpu, mem);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_reti(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   reti", cpu->start_pc);

    cpu->ime = true;
    cpu->pc = cpu_stack_pop_u16(cpu, mem);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_jp_hl(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   jp hl", cpu->start_pc);

    cpu->pc = cpu_read_rp(cpu, RP_HL);
}

static inline void cpu_instr_ld_sp_hl(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   ld sp, hl", cpu->start_pc);

    cpu->sp = cpu_read_rp(cpu, RP_HL);
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_ldh_c_a(Cpu *cpu, Memory mem)
{
    sink_log(cpu->sink, "%04X   ldh [c], a", cpu->start_pc);

    u16 addr = 0xFF00 + cpu->c;
    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_ld_a16_a(Cpu *cpu, Memory mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   ld [$%04X], a", cpu->start_pc, addr);

    cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void cpu_instr_ldh_a_c(Cpu *cpu, Memory mem)
{
    u16 addr = 0xFF00 + cpu->c;
    sink_log(cpu->sink, "%04X   ld a, [c]", cpu->start_pc);

    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_ld_a_a16(Cpu *cpu, Memory mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   ld a, [$%04X]", cpu->start_pc, addr);

    cpu->a = cpu_read_mem(cpu, mem, addr);
}

static inline void cpu_instr_jp_cc_a16(Cpu *cpu, Memory mem, u8 y)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   jp %s, $%04X", cpu->start_pc, NAME_CC[y], addr);

    if (cpu_read_cc(cpu, y)) {
        cpu->pc = addr;
        cpu->mcycle_cnt++;
    }
}

static inline void cpu_instr_jp_a16(Cpu *cpu, Memory mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   jp $%04X", cpu->pc - 3, addr);

    cpu->pc = addr;
    cpu->mcycle_cnt++;
}

static inline void cpu_instr_di(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   di", cpu->start_pc);

    cpu->ime = false;
    cpu->queued_ime = false;
}

static inline void cpu_instr_ei(Cpu *cpu)
{
    sink_log(cpu->sink, "%04X   ei", cpu->start_pc);

    cpu->queued_ime = true;
}

static inline void cpu_instr_call_cc_n16(Cpu *cpu, Memory mem, u8 y)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   call %s, $%04X", cpu->start_pc, NAME_CC[y],
             addr);

    if (cpu_read_cc(cpu, y)) {
        cpu_stack_push_u16(cpu, mem, cpu->pc);
        cpu->pc = addr;
    }
}

static inline void cpu_instr_push_r16(Cpu *cpu, Memory mem, u8 p)
{
    sink_log(cpu->sink, "%04X   push %s", cpu->start_pc, NAME_RP2[p]);

    u16 value = cpu_read_rp2(cpu, p);
    cpu_stack_push_u16(cpu, mem, value);
}

static inline void cpu_instr_call_n16(Cpu *cpu, Memory mem)
{
    u16 addr = cpu_read_pc_u16(cpu, mem);
    sink_log(cpu->sink, "%04X   call $%04X (sp = %04X)", cpu->start_pc, addr,
             cpu->sp);

    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = addr;
}

static inline void cpu_instr_alu_a_a8(Cpu *cpu, Memory mem, u8 y)
{
    u8 rhs = cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "%04X   %s a, $%02X", cpu->start_pc, NAME_ALU[y], rhs);

    cpu_instr_alu(cpu, y, rhs);
}

static inline void cpu_instr_rst_vec(Cpu *cpu, Memory mem, u8 y)
{
    sink_log(cpu->sink, "%04X   rst $%02X", cpu->start_pc, y * 8);

    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = y << 3;
}

static inline void cpu_instr_rlc_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   rlc %s", cpu->start_pc, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_7 = (value & 0x80) != 0;
    u8 new_value = (value << 1) | bit_7;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_rrc_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   rrc %s", cpu->start_pc, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_0 = value & 1;
    u8 new_value = (value >> 1) | (bit_0 << 7);
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_rl_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   rl %s", cpu->start_pc, NAME_R[z]);

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

static inline void cpu_instr_rr_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   rr %s", cpu->start_pc, NAME_R[z]);

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

static inline void cpu_instr_sla_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   sla %s", cpu->start_pc, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_7 = (value & 0x80) != 0;
    u8 new_value = value << 1;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_7);
}

static inline void cpu_instr_sra_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   sra %s", cpu->start_pc, NAME_R[z]);

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

static inline void cpu_instr_swap_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   swap %s", cpu->start_pc, NAME_R[z]);

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

static inline void cpu_instr_srl_r8(Cpu *cpu, Memory mem, u8 z)
{
    sink_log(cpu->sink, "%04X   srl %s", cpu->start_pc, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    u8 bit_0 = value & 1;
    u8 new_value = value >> 1;
    cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CPU_FLAG_Z, new_value == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, false);
    set_bits(&cpu->f, CPU_FLAG_C, bit_0);
}

static inline void cpu_instr_bit_u3_r8(Cpu *cpu, Memory mem, u8 y, u8 z)
{
    sink_log(cpu->sink, "%04X   bit %d,%s", cpu->start_pc, y, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    set_bits(&cpu->f, CPU_FLAG_Z, (value & (1 << y)) == 0);
    set_bits(&cpu->f, CPU_FLAG_N, false);
    set_bits(&cpu->f, CPU_FLAG_H, true);
}

static inline void cpu_instr_res_u3_r8(Cpu *cpu, Memory mem, u8 y, u8 z)
{
    sink_log(cpu->sink, "%04X   res %d,%s", cpu->start_pc, y, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    cpu_write_r(cpu, mem, z, value & ~(1 << y));
}

static inline void cpu_instr_set_u3_r8(Cpu *cpu, Memory mem, u8 y, u8 z)
{
    sink_log(cpu->sink, "%04X   set %d,%s", cpu->start_pc, y, NAME_R[z]);

    u8 value = cpu_read_r(cpu, mem, z);
    cpu_write_r(cpu, mem, z, value | (1 << y));
}

static inline void cpu_instr_prefix(Cpu *cpu, Memory mem)
{
    u8 opcode = cpu_read_pc(cpu, mem);
    sink_log(cpu->sink, "{prefix} $%02X", opcode);
    sink_log(cpu->sink, "    prefixed (opcode = $%02X)", opcode);

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

void cpu_execute(Cpu *cpu, Memory mem, u8 opcode)
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
                        case 0: cpu_instr_nop(cpu); break;
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
                        default: BAIL("removed instruction (opcode = $%02X)", opcode);
                    }
                    // clang-format on
                    break;
                case 4:
                    if (y < 4)
                        cpu_instr_call_cc_n16(cpu, mem, y);
                    else
                        BAIL("removed instruction (opcode = $%02X)", opcode);
                    break;
                case 5:
                    if (q == 0)
                        cpu_instr_push_r16(cpu, mem, p);
                    else if (p == 0)
                        cpu_instr_call_n16(cpu, mem);
                    else
                        BAIL("removed instruction (opcode = $%02X)", opcode);
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

void cpu_step(Cpu *cpu, Memory mem)
{
    if (cpu->mode != CPU_MODE_RUNNING) {
        ++cpu->mcycle_cnt; // Makes the frontend work lmao
        return;
    }

    if (cpu->queued_ime) {
        cpu->ime = true;
        cpu->queued_ime = false;
    }

    cpu->start_pc = cpu->pc;
    u8 opcode = cpu_read_pc(cpu, mem);
    cpu_execute(cpu, mem, opcode);
}

bool cpu_interrupt(Cpu *cpu, Memory mem, u8 handler_location)
{
    if (!cpu->ime)
        return false;

    cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->ime = false;
    cpu->pc = handler_location;
    cpu->mcycle_cnt += 2;
    return true;
}
