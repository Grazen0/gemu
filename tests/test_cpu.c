#include "cpu.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unity.h>

void test_cpu_new()
{
    Cpu cpu = cpu_init();

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
