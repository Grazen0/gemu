#include "cpu.h"
#include "instructions.h"
#include "macros.h"
#include "num.h"
#include "stdinc.h"

Cpu Cpu_new()
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

bool Cpu_read_cc(const Cpu *const self, const CpuTableCc cc)
{
    switch (cc) {
    case CpuTableCc_NZ:
        return (self->f & CpuFlag_Z) == 0;
    case CpuTableCc_Z:
        return (self->f & CpuFlag_Z) != 0;
    case CpuTableCc_NC:
        return (self->f & CpuFlag_C) == 0;
    case CpuTableCc_C:
        return (self->f & CpuFlag_C) != 0;
    default:
        BAIL("invalid cc: %i", cc);
    }
}

u16 Cpu_read_rp(const Cpu *const self, const CpuTableRp rp)
{
    switch (rp) {
    case CpuTableRp_BC:
        return concat_u16(self->b, self->c);
    case CpuTableRp_DE:
        return concat_u16(self->d, self->e);
    case CpuTableRp_HL:
        return concat_u16(self->h, self->l);
    case CpuTableRp_SP:
        return self->sp;
    default:
        BAIL("invalid rp: %i", rp);
    }
}

void Cpu_write_rp(Cpu *const self, const CpuTableRp rp, const u16 value)
{
    switch (rp) {
    case CpuTableRp_BC:
        self->b = value >> 8;
        self->c = value & 0xFF;
        break;
    case CpuTableRp_DE:
        self->d = value >> 8;
        self->e = value & 0xFF;
        break;
    case CpuTableRp_HL:
        self->h = value >> 8;
        self->l = value & 0xFF;
        break;
    case CpuTableRp_SP:
        self->sp = value;
        break;
    default:
        BAIL("invalid rp: %i", rp);
    }
}

u16 Cpu_read_rp2(const Cpu *const self, const CpuTableRp rp)
{
    switch (rp) {
    case CpuTableRp2_BC:
        return concat_u16(self->b, self->c);
    case CpuTableRp2_DE:
        return concat_u16(self->d, self->e);
    case CpuTableRp2_HL:
        return concat_u16(self->h, self->l);
    case CpuTableRp2_AF:
        return concat_u16(self->a, self->f);
    default:
        BAIL("invalid rp2: %i", rp);
    }
}

void Cpu_write_rp2(Cpu *const self, const CpuTableRp rp, const u16 value)
{
    switch (rp) {
    case CpuTableRp2_BC:
        self->b = value >> 8;
        self->c = value & 0xFF;
        break;
    case CpuTableRp2_DE:
        self->d = value >> 8;
        self->e = value & 0xFF;
        break;
    case CpuTableRp2_HL:
        self->h = value >> 8;
        self->l = value & 0xFF;
        break;
    case CpuTableRp2_AF:
        self->a = value >> 8;
        self->f = value & 0xF0;
        break;
    default:
        BAIL("invalid rp2: %i", rp);
    }
}

u8 Cpu_read_mem(Cpu *const self, const Memory *const mem, const u16 addr)
{
    self->cycle_count++;
    return mem->read(mem->ctx, addr);
}

u16 Cpu_read_mem_u16(Cpu *const self, const Memory *const mem, const u16 addr)
{
    const u8 lo = Cpu_read_mem(self, mem, addr);
    const u8 hi = Cpu_read_mem(self, mem, addr + 1);
    return concat_u16(hi, lo);
}

void Cpu_write_mem(Cpu *const self, Memory *const mem, const u16 addr,
                   const u8 value)
{
    self->cycle_count++;
    mem->write(mem->ctx, addr, value);
}

void Cpu_write_mem_u16(Cpu *const self, Memory *const mem, const u16 addr,
                       const u16 value)
{
    Cpu_write_mem(self, mem, addr, value & 0xFF);
    Cpu_write_mem(self, mem, addr + 1, value >> 8);
}

u8 Cpu_read_pc(Cpu *const self, const Memory *const mem)
{
    const u8 value = Cpu_read_mem(self, mem, self->pc);
    self->pc++;
    return value;
}

u16 Cpu_read_pc_u16(Cpu *const self, const Memory *const mem)
{
    const u16 value = Cpu_read_mem_u16(self, mem, self->pc);
    self->pc += 2;
    return value;
}

void Cpu_stack_push_u16(Cpu *const self, Memory *const mem, const u16 value)
{
    self->sp -= 2;
    Cpu_write_mem_u16(self, mem, self->sp, value);
    self->cycle_count++;
}

u16 Cpu_stack_pop_u16(Cpu *const self, const Memory *mem)
{
    const u16 value = Cpu_read_mem_u16(self, mem, self->sp);
    self->sp += 2;
    return value;
}

u8 Cpu_read_r(Cpu *const self, const Memory *const mem, const CpuTableR r)
{
    switch (r) {
    case CpuTableR_B:
        return self->b;
    case CpuTableR_C:
        return self->c;
    case CpuTableR_D:
        return self->d;
    case CpuTableR_E:
        return self->e;
    case CpuTableR_H:
        return self->h;
    case CpuTableR_L:
        return self->l;
    case CpuTableR_HL: {
        const u16 addr = Cpu_read_rp(self, CpuTableRp_HL);
        return Cpu_read_mem(self, mem, addr);
    }
    case CpuTableR_A:
        return self->a;
    default:
        BAIL("invalid cpu r: %i", r);
    }
}

void Cpu_write_r(Cpu *const self, Memory *const mem, const CpuTableR r,
                 const u8 value)
{
    switch (r) {
    case CpuTableR_B:
        self->b = value;
        break;
    case CpuTableR_C:
        self->c = value;
        break;
    case CpuTableR_D:
        self->d = value;
        break;
    case CpuTableR_E:
        self->e = value;
        break;
    case CpuTableR_H:
        self->h = value;
        break;
    case CpuTableR_L:
        self->l = value;
        break;
    case CpuTableR_HL: {
        const u16 addr = Cpu_read_rp(self, CpuTableRp_HL);
        Cpu_write_mem(self, mem, addr, value);
        break;
    }
    case CpuTableR_A:
        self->a = value;
        break;
    default:
        BAIL("[GameBoy_write_r] invalid CpuTableR r: %i", r);
    }
}

void Cpu_tick(Cpu *const self, Memory *const mem)
{
    if (self->mode != CpuMode_Running) {
        self->cycle_count++; // Makes the frontend work lmao
        return;
    }

    if (self->queued_ime) {
        self->ime = true;
        self->queued_ime = false;
    }

    const u8 opcode = Cpu_read_pc(self, mem);
    Cpu_execute(self, mem, opcode);
}

void Cpu_interrupt(Cpu *const self, Memory *const mem,
                   const u8 handler_location)
{
    Cpu_stack_push_u16(self, mem, self->pc);
    self->ime = false;
    self->pc = handler_location;
    self->cycle_count += 2;
}
