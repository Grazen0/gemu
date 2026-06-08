#include "cpu.h"
#include <unity.h>

static void nop_vlog_v([[maybe_unused]] void *logger,
                       [[maybe_unused]] const char format[],
                       [[maybe_unused]] va_list args)
{
}

static const LoggerVTable NOP_LOGGER_VTABLE = {
    .vlog = nop_vlog_v,
};

static const Logger NOP_LOGGER = {
    .ptr = nullptr,
    .vtable = &NOP_LOGGER_VTABLE,
};

void test_cpu_init()
{
    Cpu cpu = cpu_init(NOP_LOGGER);

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
    TEST_ASSERT_EQUAL(cpu.mode, CPU_MODE_RUNNING);
    TEST_ASSERT_EQUAL(cpu.ime, true);
    TEST_ASSERT_EQUAL(cpu.mcycle_cnt, 0);
}
