#include "cpu.h"
#include "control.h"
#include "instructions.h"
#include "num.h"
#include "stdinc.h"

Cpu Cpu_new(void)
{
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
        .mode = CpuMode_Running,
        .queued_ime = false,
        .ime = true,
        .cycle_count = 0,
    };
}

bool Cpu_read_cc(const Cpu *const restrict cpu, const CpuTableCc cc)
{
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

u16 Cpu_read_rp(const Cpu *const restrict cpu, const CpuTableRp rp)
{
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

void Cpu_write_rp(Cpu *const restrict cpu, const CpuTableRp rp, const u16 value)
{
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

u16 Cpu_read_rp2(const Cpu *const restrict cpu, const CpuTableRp rp)
{
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

void Cpu_write_rp2(Cpu *const restrict cpu, const CpuTableRp rp,
                   const u16 value)
{
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

u8 Cpu_read_mem(Cpu *const restrict cpu, const Memory *const restrict mem,
                const u16 addr)
{
    cpu->cycle_count++;
    return mem->read(mem->ctx, addr);
}

u16 Cpu_read_mem_u16(Cpu *const restrict cpu, const Memory *const restrict mem,
                     const u16 addr)
{
    const u8 lo = Cpu_read_mem(cpu, mem, addr);
    const u8 hi = Cpu_read_mem(cpu, mem, addr + 1);
    return concat_u16(hi, lo);
}

void Cpu_write_mem(Cpu *const restrict cpu, Memory *const restrict mem,
                   const u16 addr, const u8 value)
{
    cpu->cycle_count++;
    mem->write(mem->ctx, addr, value);
}

void Cpu_write_mem_u16(Cpu *const restrict cpu, Memory *const restrict mem,
                       const u16 addr, const u16 value)
{
    Cpu_write_mem(cpu, mem, addr, value & 0xFF);
    Cpu_write_mem(cpu, mem, addr + 1, value >> 8);
}

u8 Cpu_read_pc(Cpu *const restrict cpu, const Memory *const restrict mem)
{
    const u8 value = Cpu_read_mem(cpu, mem, cpu->pc);
    cpu->pc++;
    return value;
}

u16 Cpu_read_pc_u16(Cpu *const restrict cpu, const Memory *const restrict mem)
{
    const u16 value = Cpu_read_mem_u16(cpu, mem, cpu->pc);
    cpu->pc += 2;
    return value;
}

void Cpu_stack_push_u16(Cpu *const restrict cpu, Memory *const restrict mem,
                        const u16 value)
{
    cpu->sp -= 2;
    Cpu_write_mem_u16(cpu, mem, cpu->sp, value);
    cpu->cycle_count++;
}

u16 Cpu_stack_pop_u16(Cpu *const restrict cpu, const Memory *restrict mem)
{
    const u16 value = Cpu_read_mem_u16(cpu, mem, cpu->sp);
    cpu->sp += 2;
    return value;
}

u8 Cpu_read_r(Cpu *const restrict cpu, const Memory *const restrict mem,
              const CpuTableR r)
{
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
        const u16 addr = Cpu_read_rp(cpu, CpuTableRp_HL);
        return Cpu_read_mem(cpu, mem, addr);
    }
    case CpuTableR_A:
        return cpu->a;
    default:
        BAIL("invalid cpu r: %i", r);
    }
}

void Cpu_write_r(Cpu *const restrict cpu, Memory *const restrict mem,
                 const CpuTableR r, const u8 value)
{
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
        const u16 addr = Cpu_read_rp(cpu, CpuTableRp_HL);
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

void Cpu_tick(Cpu *const restrict cpu, Memory *const restrict mem)
{
    if (cpu->mode != CpuMode_Running) {
        cpu->cycle_count++; // Makes the frontend work lmao
        return;
    }

    if (cpu->queued_ime) {
        cpu->ime = true;
        cpu->queued_ime = false;
    }

    const u8 opcode = Cpu_read_pc(cpu, mem);
    Cpu_execute(cpu, mem, opcode);
}

void Cpu_interrupt(Cpu *const restrict cpu, Memory *const restrict mem,
                   const u8 handler_location)
{
    Cpu_stack_push_u16(cpu, mem, cpu->pc);
    cpu->ime = false;
    cpu->pc = handler_location;
    cpu->cycle_count += 2;
}
