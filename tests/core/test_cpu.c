#include <stdint.h>
#include <string.h>
#include "core/cpu.h"
#include "unity.h"

static Cpu cpu;
static uint8_t mem_data[0x10000];
static Memory mock_memory;

static uint8_t read_mock_mem(const void* ctx, const uint16_t addr) {
    const uint8_t* const restrict memory = ctx;
    return memory[addr];
}

static void write_mock_mem(void* ctx, const uint16_t addr, const uint8_t value) {
    uint8_t* const restrict memory = ctx;
    memory[addr] = value;
}

void setUp(void) {
    cpu = Cpu_new();
    mock_memory = (Memory){
        .ctx = mem_data,
        .read = read_mock_mem,
        .write = write_mock_mem,
    };
    memset(mem_data, 0, sizeof(mem_data));
}

void test_cpu_new(void) {
    TEST_ASSERT_EQUAL(cpu.b, 0);
    TEST_ASSERT_EQUAL(cpu.c, 0);
    TEST_ASSERT_EQUAL(cpu.d, 0);
    TEST_ASSERT_EQUAL(cpu.e, 0);
    TEST_ASSERT_EQUAL(cpu.h, 0);
    TEST_ASSERT_EQUAL(cpu.l, 0);
    TEST_ASSERT_EQUAL(cpu.a, 0);
    TEST_ASSERT_EQUAL(cpu.f, 0);
    TEST_ASSERT_EQUAL(cpu.sp, 0);
    TEST_ASSERT_EQUAL(cpu.pc, 0);
    TEST_ASSERT_EQUAL(cpu.halted, false);
    TEST_ASSERT_EQUAL(cpu.ime, true);
    TEST_ASSERT_EQUAL(cpu.cycle_count, 0);
}

void test_cpu_write_mem(void) {
    Cpu_write_mem(&cpu, &mock_memory, 0x0000, 0x42);
    TEST_ASSERT_EQUAL_HEX(0x42, mem_data[0x0000]);
}
