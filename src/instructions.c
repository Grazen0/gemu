#include "instructions.h"
#include "control.h"
#include "cpu.h"
#include "log.h"
#include "num.h"
#include "stdinc.h"

static inline void Cpu_instr_add_u8(Cpu *const cpu, const u8 rhs)
{
    const u8 prev_a = cpu->a;
    cpu->a += rhs;

    set_bits(&cpu->f, CpuFlag_C, rhs > (0xFF - prev_a));
    set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) + (rhs & 0xF) > 0xF);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
}

static inline void Cpu_instr_adc_u8(Cpu *const cpu, const u8 rhs)
{
    const u8 prev_a = cpu->a;
    const u8 carry = (cpu->f & CpuFlag_C) != 0;
    const u16 result = (u16)prev_a + rhs + carry;
    cpu->a = (u8)result;

    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) + (rhs & 0xF) + carry > 0xF);
    set_bits(&cpu->f, CpuFlag_C, result > 0xFF);
}

static inline void Cpu_instr_sub_u8(Cpu *const cpu, const u8 rhs)
{
    const u8 prev_a = cpu->a;
    cpu->a -= rhs;

    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, true);
    set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) < (rhs & 0xF));
    set_bits(&cpu->f, CpuFlag_C, rhs > prev_a);
}

static inline void Cpu_instr_sbc_u8(Cpu *const cpu, const u8 rhs)
{
    const u8 prev_a = cpu->a;
    const u8 borrow = (cpu->f & CpuFlag_C) != 0;
    cpu->a = prev_a - rhs - borrow;

    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, true);
    set_bits(&cpu->f, CpuFlag_H, (prev_a & 0xF) < (rhs & 0xF) + borrow);
    set_bits(&cpu->f, CpuFlag_C, prev_a < (u16)rhs + borrow);
}

static inline void Cpu_instr_and_u8(Cpu *const cpu, const u8 rhs)
{
    cpu->a &= rhs;
    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, true);
    set_bits(&cpu->f, CpuFlag_C, false);
}

static inline void Cpu_instr_xor_u8(Cpu *const cpu, const u8 rhs)
{
    cpu->a ^= rhs;
    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, false);
}

static inline void Cpu_instr_or_u8(Cpu *const cpu, const u8 rhs)
{
    cpu->a |= rhs;
    set_bits(&cpu->f, CpuFlag_Z, cpu->a == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, false);
}

static inline void Cpu_instr_cp_u8(Cpu *const cpu, const u8 rhs)
{
    set_bits(&cpu->f, CpuFlag_Z, cpu->a == rhs);
    set_bits(&cpu->f, CpuFlag_N, true);
    set_bits(&cpu->f, CpuFlag_H, (cpu->a & 0xF) < (rhs & 0xF));
    set_bits(&cpu->f, CpuFlag_C, cpu->a < rhs);
}

static inline void Cpu_instr_alu(Cpu *const cpu, const CpuTableAlu alu,
                                 const u8 rhs)
{
    // clang-format off
    switch (alu) {
        case CpuTableAlu_Add: Cpu_instr_add_u8(cpu, rhs); break;
        case CpuTableAlu_Adc: Cpu_instr_adc_u8(cpu, rhs); break;
        case CpuTableAlu_Sub: Cpu_instr_sub_u8(cpu, rhs); break;
        case CpuTableAlu_Sbc: Cpu_instr_sbc_u8(cpu, rhs); break;
        case CpuTableAlu_And: Cpu_instr_and_u8(cpu, rhs); break;
        case CpuTableAlu_Xor: Cpu_instr_xor_u8(cpu, rhs); break;
        case CpuTableAlu_Or: Cpu_instr_or_u8(cpu, rhs); break;
        case CpuTableAlu_Cp: Cpu_instr_cp_u8(cpu, rhs); break;
        default: BAIL("invalid alu: %i", alu);
    }
    // clang-format on
}

static inline void Cpu_instr_nop()
{
    log_trace("nop");
}

static inline void Cpu_instr_ld_n16_sp(Cpu *const cpu,
                                       Memory *const mem)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("ld [$%04X], SP", addr);

    Cpu_write_mem_u16(cpu, mem, addr, cpu->sp);
}

static inline void Cpu_instr_stop(Cpu *const cpu)
{
    log_trace("stop");
    cpu->mode = CpuMode_Stopped;

    log_debug("TODO: implement STOP instruction properly");
}

static inline void Cpu_instr_jr_e8(Cpu *const cpu,
                                   const Memory *const mem)
{
    const i8 offset = (i8)Cpu_read_pc(cpu, mem);
    log_trace("jr %i", offset);

    cpu->pc += offset;
    cpu->cycle_count++;
}

static inline void Cpu_instr_jr_cc_e8(Cpu *const cpu,
                                      const Memory *const mem,
                                      const u8 y)
{
    const u8 cc = y - 4;
    const i8 offset = (i8)Cpu_read_pc(cpu, mem);
    log_trace("jr cc(%i), %i", cc, offset);

    if (Cpu_read_cc(cpu, cc)) {
        cpu->pc += offset;
        cpu->cycle_count++;
    }
}

static inline void Cpu_instr_ld_r16_n16(Cpu *const cpu,
                                        const Memory *const mem,
                                        const u8 p)
{
    const u16 value = Cpu_read_pc_u16(cpu, mem);
    log_trace("ld rp(%d), $%04X", p, value);

    Cpu_write_rp(cpu, p, value);
}

static inline void Cpu_instr_add_hl_r16(Cpu *const cpu, const u8 p)
{
    log_trace("add hl, rp(%d)", p);

    const u16 hl = Cpu_read_rp(cpu, CpuTableRp_HL);
    const u16 rhs = Cpu_read_rp(cpu, p);

    Cpu_write_rp(cpu, CpuTableRp_HL, hl + rhs);

    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, (hl & 0xFFF) + (rhs & 0xFFF) > 0xFFF);
    set_bits(&cpu->f, CpuFlag_C, rhs > 0xFFFF - hl);

    cpu->cycle_count++;
}

static inline void Cpu_instr_ld_bc_a(Cpu *const cpu,
                                     Memory *const mem)
{
    log_trace("ld [bc], a");

    const u16 bc = Cpu_read_rp(cpu, CpuTableRp_BC);
    Cpu_write_mem(cpu, mem, bc, cpu->a);
}

static inline void Cpu_instr_ld_de_a(Cpu *const cpu,
                                     Memory *const mem)
{
    log_trace("ld [de], a");

    const u16 de = Cpu_read_rp(cpu, CpuTableRp_DE);
    Cpu_write_mem(cpu, mem, de, cpu->a);
}

static inline void Cpu_instr_ld_hli_a(Cpu *const cpu,
                                      Memory *const mem)
{
    log_trace("ld [hl+], a");

    const u16 hl = Cpu_read_rp(cpu, CpuTableRp_HL);
    Cpu_write_mem(cpu, mem, hl, cpu->a);
    Cpu_write_rp(cpu, CpuTableRp_HL, hl + 1);
}

static inline void Cpu_instr_ld_hld_a(Cpu *const cpu,
                                      Memory *const mem)
{
    log_trace("ld [hl-], a");

    const u16 hl = Cpu_read_rp(cpu, CpuTableRp_HL);
    Cpu_write_mem(cpu, mem, hl, cpu->a);
    Cpu_write_rp(cpu, CpuTableRp_HL, hl - 1);
}

static inline void Cpu_instr_ld_a_bc(Cpu *const cpu,
                                     const Memory *const mem)
{
    log_trace("ld a, [bc]");
    const u16 bc = Cpu_read_rp(cpu, CpuTableRp_BC);
    cpu->a = Cpu_read_mem(cpu, mem, bc);
}

static inline void Cpu_instr_ld_a_de(Cpu *const cpu,
                                     const Memory *const mem)
{
    log_trace("ld a, [de]");

    const u16 de = Cpu_read_rp(cpu, CpuTableRp_DE);
    cpu->a = Cpu_read_mem(cpu, mem, de);
}

static inline void Cpu_instr_ld_a_hli(Cpu *const cpu,
                                      const Memory *const mem)
{
    log_trace("ld a, [hl+]");

    const u16 hl = Cpu_read_rp(cpu, CpuTableRp_HL);
    cpu->a = Cpu_read_mem(cpu, mem, hl);
    Cpu_write_rp(cpu, CpuTableRp_HL, hl + 1);
}

static inline void Cpu_instr_ld_a_hld(Cpu *const cpu,
                                      const Memory *const mem)
{
    log_trace("ld a, [hl-]");

    const u16 hl = Cpu_read_rp(cpu, CpuTableRp_HL);
    cpu->a = Cpu_read_mem(cpu, mem, hl);
    Cpu_write_rp(cpu, CpuTableRp_HL, hl - 1);
}

static inline void Cpu_instr_inc_r16(Cpu *const cpu, const u8 p)
{
    log_trace("inc rp(%d)", p);

    const u16 value = Cpu_read_rp(cpu, p);
    Cpu_write_rp(cpu, p, value + 1);
    cpu->cycle_count++;
}

static inline void Cpu_instr_dec_r16(Cpu *const cpu, const u8 p)
{
    log_trace("dec rp(%d)", p);

    const u16 value = Cpu_read_rp(cpu, p);
    Cpu_write_rp(cpu, p, value - 1);
    cpu->cycle_count++;
}

static inline void Cpu_instr_inc_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 y)
{
    log_trace("inc r(%d)", y);

    const u8 value = Cpu_read_r(cpu, mem, y);
    const u8 new_value = value + 1;
    Cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, (new_value & 0xF) == 0);
}

static inline void Cpu_instr_dec_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 y)
{
    log_trace("dec r(%d)", y);

    const u8 value = Cpu_read_r(cpu, mem, y);
    const u8 new_value = value - 1;
    Cpu_write_r(cpu, mem, y, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, true);
    set_bits(&cpu->f, CpuFlag_H, (new_value & 0xF) == 0xF);
}

static inline void Cpu_instr_ld_r8_n(Cpu *const cpu,
                                     Memory *const mem, const u8 y)
{
    const u8 value = Cpu_read_pc(cpu, mem);
    log_trace("ld r(%d), $%02X", y, value);

    Cpu_write_r(cpu, mem, y, value);
}

static inline void Cpu_instr_rlca(Cpu *const cpu)
{
    log_trace("rlca");

    const u8 bit_7 = (cpu->a & 0x80) != 0;
    cpu->a = (cpu->a << 1) | bit_7;

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_7);
}

static inline void Cpu_instr_rrca(Cpu *const cpu)
{
    log_trace("rrca");

    const u8 bit_0 = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (bit_0 << 7);

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_0);
}

static inline void Cpu_instr_rla(Cpu *const cpu)
{
    log_trace("rla");

    const u8 prev_carry = (cpu->f & CpuFlag_C) != 0;
    const u8 new_carry = (cpu->a & 0x80) != 0;
    cpu->a = (cpu->a << 1) | prev_carry;

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, new_carry);
}

static inline void Cpu_instr_rra(Cpu *const cpu)
{
    log_trace("rra");

    const u8 prev_carry = (cpu->f & CpuFlag_C) != 0;
    const u8 new_carry = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (prev_carry << 7);

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, new_carry);
}

static inline void Cpu_instr_daa(Cpu *const cpu)
{
    log_trace("daa");

    u8 adj = 0;

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
}

static inline void Cpu_instr_cpl(Cpu *const cpu)
{
    log_trace("cpl");

    cpu->a = ~cpu->a;
    set_bits(&cpu->f, CpuFlag_N, true);
    set_bits(&cpu->f, CpuFlag_H, true);
}

static inline void Cpu_instr_scf(Cpu *const cpu)
{
    log_trace("scf");

    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, true);
}

static inline void Cpu_instr_ccf(Cpu *const cpu)
{
    log_trace("ccf");

    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, !(cpu->f & CpuFlag_C));
}

static inline void Cpu_instr_halt(Cpu *const cpu)
{
    log_trace("halt");
    cpu->mode = CpuMode_Halted;
}

static inline void Cpu_instr_ld_r8_r8(Cpu *const cpu,
                                      Memory *const mem, const u8 y,
                                      const u8 z)
{
    const u8 value = Cpu_read_r(cpu, mem, z);
    log_trace("ld r(%d), r(%d)", y, z);

    Cpu_write_r(cpu, mem, y, value);
}

static inline void Cpu_instr_alu_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 y,
                                    const u8 z)
{

    log_trace("{alu} a, r(%d)", z);

    const u8 rhs = Cpu_read_r(cpu, mem, z);
    Cpu_instr_alu(cpu, y, rhs);
}

static inline void Cpu_instr_ldh_n16_a(Cpu *const cpu,
                                       Memory *const mem)
{
    const u8 offset = Cpu_read_pc(cpu, mem);
    log_trace("ldh [$%02X], a", offset);

    const u16 addr = 0xFF00 + offset;
    Cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void Cpu_instr_add_sp_e8(Cpu *const cpu,
                                       Memory *const mem)
{
    const u8 offset_u8 = (i8)Cpu_read_pc(cpu, mem);
    const i8 offset = (i8)offset_u8;
    log_trace("add sp, %d", offset);

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CpuFlag_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    cpu->sp += offset;
    cpu->cycle_count += 2;
}

static inline void Cpu_instr_ldh_a_n16(Cpu *const cpu,
                                       const Memory *const mem)
{
    const u8 offset = Cpu_read_pc(cpu, mem);
    log_trace("ldh a, [$%02X]", offset);

    const u16 addr = 0xFF00 + offset;
    cpu->a = Cpu_read_mem(cpu, mem, addr);
}

static inline void Cpu_instr_ld_hl_sp_plus_e8(Cpu *const cpu,
                                              const Memory *const mem)
{
    const u8 offset_u8 = (i8)Cpu_read_pc(cpu, mem);
    const i8 offset = (i8)offset_u8;
    log_trace("ld hl, sp%+d", offset);

    set_bits(&cpu->f, CpuFlag_Z, false);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, (cpu->sp & 0xF) + (offset_u8 & 0xF) > 0xF);
    set_bits(&cpu->f, CpuFlag_C, (cpu->sp & 0xFF) + offset_u8 > 0xFF);

    Cpu_write_rp(cpu, CpuTableRp_HL, cpu->sp + offset);
    cpu->cycle_count++;
}

static inline void Cpu_instr_ret_cc(Cpu *const cpu,
                                    Memory *const mem, const u8 y)
{
    log_trace("ret cc(%d)", y);

    cpu->cycle_count++;
    if (Cpu_read_cc(cpu, y)) {
        cpu->pc = Cpu_stack_pop_u16(cpu, mem);
        cpu->cycle_count++;
    }
}

static inline void Cpu_instr_pop_r16(Cpu *const cpu,
                                     Memory *const mem, const u8 p)
{
    log_trace("pop rp2(%d)", p);

    const u16 value = Cpu_stack_pop_u16(cpu, mem);
    Cpu_write_rp2(cpu, p, value);
}

static inline void Cpu_instr_ret(Cpu *const cpu,
                                 Memory *const mem)
{
    log_trace("ret");

    cpu->pc = Cpu_stack_pop_u16(cpu, mem);
    cpu->cycle_count++;
}

static inline void Cpu_instr_reti(Cpu *const cpu,
                                  Memory *const mem)
{
    log_trace("reti");

    cpu->ime = true;
    cpu->pc = Cpu_stack_pop_u16(cpu, mem);
    cpu->cycle_count++;
}

static inline void Cpu_instr_jp_hl(Cpu *const cpu)
{
    log_trace("jp hl");

    cpu->pc = Cpu_read_rp(cpu, CpuTableRp_HL);
}

static inline void Cpu_instr_ld_sp_hl(Cpu *const cpu)
{
    log_trace("ld sp, hl");

    cpu->sp = Cpu_read_rp(cpu, CpuTableRp_HL);
    cpu->cycle_count++;
}

static inline void Cpu_instr_ldh_c_a(Cpu *const cpu,
                                     Memory *const mem)
{
    log_trace("ldh [c], a");

    const u16 addr = 0xFF00 + cpu->c;
    Cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void Cpu_instr_ld_a16_a(Cpu *const cpu,
                                      Memory *const mem)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("ld [$%04X], a", addr);

    Cpu_write_mem(cpu, mem, addr, cpu->a);
}

static inline void Cpu_instr_ldh_a_c(Cpu *const cpu,
                                     const Memory *const mem)
{
    const u16 addr = 0xFF00 + cpu->c;
    log_trace("ld a, [c]");

    cpu->a = Cpu_read_mem(cpu, mem, addr);
}

static inline void Cpu_instr_ld_a_a16(Cpu *const cpu,
                                      const Memory *const mem)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("ld a, [$%04X]", addr);

    cpu->a = Cpu_read_mem(cpu, mem, addr);
}

static inline void Cpu_instr_jp_cc_a16(Cpu *const cpu,
                                       const Memory *const mem,
                                       const u8 y)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("jp cc(%d), $%04X", y, addr);

    if (Cpu_read_cc(cpu, y)) {
        cpu->pc = addr;
        cpu->cycle_count++;
    }
}

static inline void Cpu_instr_jp_a16(Cpu *const cpu,
                                    const Memory *const mem)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("jp $%04X", addr);

    cpu->pc = addr;
    cpu->cycle_count++;
}

static inline void Cpu_instr_di(Cpu *const cpu)
{
    log_trace("di");

    cpu->ime = false;
    cpu->queued_ime = false;
}

static inline void Cpu_instr_ei(Cpu *const cpu)
{
    log_trace("ei");

    cpu->queued_ime = true;
}

static inline void Cpu_instr_call_cc_n16(Cpu *const cpu,
                                         Memory *const mem, const u8 y)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("call cc(%d), $%04X", y, addr);

    if (Cpu_read_cc(cpu, y)) {
        Cpu_stack_push_u16(cpu, mem, cpu->pc);
        cpu->pc = addr;
    }
}

static inline void Cpu_instr_push_r16(Cpu *const cpu,
                                      Memory *const mem, const u8 p)
{
    log_trace("push rp2(%d)", p);

    const u16 value = Cpu_read_rp2(cpu, p);
    Cpu_stack_push_u16(cpu, mem, value);
}

static inline void Cpu_instr_call_n16(Cpu *const cpu,
                                      Memory *const mem)
{
    const u16 addr = Cpu_read_pc_u16(cpu, mem);
    log_trace("call $%04X", addr);

    Cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = addr;
}

static inline void Cpu_instr_alu_a_a8(Cpu *const cpu,
                                      Memory *const mem, const u8 y)
{
    const u8 rhs = Cpu_read_pc(cpu, mem);
    log_trace("{alu} a, $%02X", rhs);

    Cpu_instr_alu(cpu, y, rhs);
}

static inline void Cpu_instr_rst_vec(Cpu *const cpu,
                                     Memory *const mem, const u8 y)
{
    log_trace("rst $%02X", y * 8);

    Cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->pc = y * 8;
}

static inline void Cpu_instr_rlc_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 z)
{
    log_trace("rlc r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 bit_7 = (value & 0x80) != 0;
    const u8 new_value = (value << 1) | bit_7;
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_7);
}

static inline void Cpu_instr_rrc_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 z)
{
    log_trace("rrc r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 bit_0 = value & 1;
    const u8 new_value = (value >> 1) | (bit_0 << 7);
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_0);
}

static inline void Cpu_instr_rl_r8(Cpu *const cpu,
                                   Memory *const mem, const u8 z)
{
    log_trace("rl r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 prev_carry = (cpu->f & CpuFlag_C) != 0;
    const u8 new_carry = (value & 0x80) != 0;

    const u8 new_value = (value << 1) | prev_carry;
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, new_carry);
}

static inline void Cpu_instr_rr_r8(Cpu *const cpu,
                                   Memory *const mem, const u8 z)
{
    log_trace("rr r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 prev_carry = (cpu->f & CpuFlag_C) != 0;
    const u8 new_carry = value & 1;

    const u8 new_value = (value >> 1) | (prev_carry << 7);
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, new_carry);
}

static inline void Cpu_instr_sla_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 z)
{
    log_trace("sla r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 bit_7 = (value & 0x80) != 0;
    const u8 new_value = value << 1;
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_7);
}

static inline void Cpu_instr_sra_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 z)
{
    log_trace("sra r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 bit_0 = value & 1;
    const u8 bit_7 = (value & 0x80) != 0;
    const u8 new_value = (value >> 1) | (bit_7 << 7);
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_0);
}

static inline void Cpu_instr_swap_r8(Cpu *const cpu,
                                     Memory *const mem, const u8 z)
{
    log_trace("swap r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 prev_hi = value >> 4;
    const u8 prev_lo = value & 0xF;
    const u8 new_value = (prev_lo << 4) | prev_hi;
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, false);
}

static inline void Cpu_instr_srl_r8(Cpu *const cpu,
                                    Memory *const mem, const u8 z)
{
    log_trace("srl r(%d)", z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    const u8 bit_0 = value & 1;
    const u8 new_value = value >> 1;
    Cpu_write_r(cpu, mem, z, new_value);

    set_bits(&cpu->f, CpuFlag_Z, new_value == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, false);
    set_bits(&cpu->f, CpuFlag_C, bit_0);
}

static inline void Cpu_instr_bit_u3_r8(Cpu *const cpu,
                                       Memory *const mem, const u8 y,
                                       const u8 z)
{
    log_trace("bit %d,r(%d)", y, z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    set_bits(&cpu->f, CpuFlag_Z, (value & (1 << y)) == 0);
    set_bits(&cpu->f, CpuFlag_N, false);
    set_bits(&cpu->f, CpuFlag_H, true);
}

static inline void Cpu_instr_res_u3_r8(Cpu *const cpu,
                                       Memory *const mem, const u8 y,
                                       const u8 z)
{
    log_trace("res %d,r(%d)", y, z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    Cpu_write_r(cpu, mem, z, value & ~(1 << y));
}

static inline void Cpu_instr_set_u3_r8(Cpu *const cpu,
                                       Memory *const mem, const u8 y,
                                       const u8 z)
{
    log_trace("set %d,r(%d)", y, z);

    const u8 value = Cpu_read_r(cpu, mem, z);
    Cpu_write_r(cpu, mem, z, value | (1 << y));
}

static inline void Cpu_instr_prefix(Cpu *const cpu,
                                    Memory *const mem)
{
    const u8 opcode = Cpu_read_pc(cpu, mem);
    log_trace("{prefix} $%02X", opcode);
    log_trace("    prefixed (opcode = $%02X)", opcode);

    const u8 x = opcode >> 6;
    const u8 y = (opcode >> 3) & 0b111;
    const u8 z = opcode & 0b111;

    // clang-format off
    switch (x) {
        case 0:
            switch (y) {
                case 0: Cpu_instr_rlc_r8(cpu, mem, z); break;
                case 1: Cpu_instr_rrc_r8(cpu, mem, z); break;
                case 2: Cpu_instr_rl_r8(cpu, mem, z); break;
                case 3: Cpu_instr_rr_r8(cpu, mem, z); break;
                case 4: Cpu_instr_sla_r8(cpu, mem, z); break;
                case 5: Cpu_instr_sra_r8(cpu, mem, z); break;
                case 6: Cpu_instr_swap_r8(cpu, mem, z); break;
                case 7: Cpu_instr_srl_r8(cpu, mem, z); break;
                default: BAIL("unreachable");
            }
            break;
        case 1: Cpu_instr_bit_u3_r8(cpu, mem, y, z); break;
        case 2: Cpu_instr_res_u3_r8(cpu, mem, y, z); break;
        case 3: Cpu_instr_set_u3_r8(cpu, mem, y, z); break;
        default: BAIL("unreachable");
    }
    // clang-format on
}

// NOLINTNEXTLINE
void Cpu_execute(Cpu *const cpu, Memory *const mem,
                 const u8 opcode)
{
    // Credit:
    // https://archive.gbdev.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html

    const u8 x = opcode >> 6;
    const u8 y = (opcode >> 3) & 0b111;
    const u8 z = opcode & 0b111;
    const u8 p = y >> 1;
    const u8 q = y & 1;

    switch (x) {
    case 0:
        switch (z) {
        case 0:
            // clang-format off
                    switch (y) {
                        case 0: Cpu_instr_nop(); break;
                        case 1: Cpu_instr_ld_n16_sp(cpu, mem); break;
                        case 2: Cpu_instr_stop(cpu); break;
                        case 3: Cpu_instr_jr_e8(cpu, mem); break;
                        default: Cpu_instr_jr_cc_e8(cpu, mem, y);
                    }
            // clang-format on
            break;
        case 1:
            if (q == 0)
                Cpu_instr_ld_r16_n16(cpu, mem, p);
            else
                Cpu_instr_add_hl_r16(cpu, p);
            break;
        case 2:
            if (q == 0) {
                // clang-format off
                        switch (p) {
                            case 0: Cpu_instr_ld_bc_a(cpu, mem); break;
                            case 1: Cpu_instr_ld_de_a(cpu, mem); break;
                            case 2: Cpu_instr_ld_hli_a(cpu, mem); break;
                            case 3: Cpu_instr_ld_hld_a(cpu, mem); break;
                            default: BAIL("unreachable");
                        }
                // clang-format on
            } else {
                // clang-format off
                        switch (p) {
                            case 0: Cpu_instr_ld_a_bc(cpu, mem); break;
                            case 1: Cpu_instr_ld_a_de(cpu, mem); break;
                            case 2: Cpu_instr_ld_a_hli(cpu, mem); break;
                            case 3: Cpu_instr_ld_a_hld(cpu, mem); break;
                            default: BAIL("unreachable");
                        }
                // clang-format on
            }
            break;
        case 3:
            if (q == 0)
                Cpu_instr_inc_r16(cpu, p);
            else
                Cpu_instr_dec_r16(cpu, p);
            break;
            // clang-format off
                case 4: Cpu_instr_inc_r8(cpu, mem, y); break;
                case 5: Cpu_instr_dec_r8(cpu, mem, y); break;
                case 6: Cpu_instr_ld_r8_n(cpu, mem, y); break;
            // clang-format on
        case 7:
            // clang-format off
                    switch (y) {
                        case 0: Cpu_instr_rlca(cpu); break;
                        case 1: Cpu_instr_rrca(cpu); break;
                        case 2: Cpu_instr_rla(cpu); break;
                        case 3: Cpu_instr_rra(cpu); break;
                        case 4: Cpu_instr_daa(cpu); break;
                        case 5: Cpu_instr_cpl(cpu); break;
                        case 6: Cpu_instr_scf(cpu); break;
                        case 7: Cpu_instr_ccf(cpu); break;
                        default: BAIL("unreachable");
                    }
            // clang-format on
            break;
        default:
            BAIL("unreachable");
        }
        break;
    case 1:
        if (z == 6 && y == 6)
            Cpu_instr_halt(cpu);
        else
            Cpu_instr_ld_r8_r8(cpu, mem, y, z);
        break;
    case 2:
        Cpu_instr_alu_r8(cpu, mem, y, z);
        break;
    case 3:
        switch (z) {
        case 0:
            // clang-format off
                    switch (y) {
                        case 4: Cpu_instr_ldh_n16_a(cpu, mem); break;
                        case 5: Cpu_instr_add_sp_e8(cpu, mem); break;
                        case 6: Cpu_instr_ldh_a_n16(cpu, mem); break;
                        case 7: Cpu_instr_ld_hl_sp_plus_e8(cpu, mem); break;
                        default: Cpu_instr_ret_cc(cpu, mem, y);
                    }
            // clang-format on
            break;
        case 1:
            if (q == 0)
                Cpu_instr_pop_r16(cpu, mem, p);
            else {
                // clang-format off
                        switch (p) {
                            case 0: Cpu_instr_ret(cpu, mem); break;
                            case 1: Cpu_instr_reti(cpu, mem); break;
                            case 2: Cpu_instr_jp_hl(cpu); break;
                            case 3: Cpu_instr_ld_sp_hl(cpu); break;
                            default: BAIL("unreachable");
                        }
                // clang-format on
            }
            break;
        case 2:
            // clang-format off
                    switch (y) {
                        case 4: Cpu_instr_ldh_c_a(cpu, mem); break;
                        case 5: Cpu_instr_ld_a16_a(cpu, mem); break;
                        case 6: Cpu_instr_ldh_a_c(cpu, mem); break;
                        case 7: Cpu_instr_ld_a_a16(cpu, mem); break;
                        default: Cpu_instr_jp_cc_a16(cpu, mem, y);
                    }
            // clang-format on
            break;
        case 3:
            // clang-format off
                    switch (y) {
                        case 0: Cpu_instr_jp_a16(cpu, mem); break;
                        case 1: Cpu_instr_prefix(cpu, mem); break;
                        case 6: Cpu_instr_di(cpu); break;
                        case 7: Cpu_instr_ei(cpu); break;
                        default: BAIL("removed instruction");
                    }
            // clang-format on
            break;
        case 4:
            if (y < 4)
                Cpu_instr_call_cc_n16(cpu, mem, y);
            else
                BAIL("removed instruction");
            break;
        case 5:
            if (q == 0)
                Cpu_instr_push_r16(cpu, mem, p);
            else if (p == 0)
                Cpu_instr_call_n16(cpu, mem);
            else
                BAIL("removed instruction");
            break;
        case 6:
            Cpu_instr_alu_a_a8(cpu, mem, y);
            break;
        case 7:
            Cpu_instr_rst_vec(cpu, mem, y);
            break;
        default:
            BAIL("unreachable");
        }
        break;
    default:
        BAIL("unreachable");
    }
}
