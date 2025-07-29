#include "common/num.h"
#include <stdint.h>
#include <unity.h>
#include <unity_internals.h>

void setUp(void) {}

void tearDown(void) {}

void test_concat_u16(void) {
    TEST_ASSERT_EQUAL_HEX(0x1234, concat_u16(0x12, 0x34));
    TEST_ASSERT_EQUAL_HEX(0x2618, concat_u16(0x26, 0x18));
    TEST_ASSERT_EQUAL_HEX(0x0000, concat_u16(0x00, 0x00));
    TEST_ASSERT_EQUAL_HEX(0xFF00, concat_u16(0xFF, 0x00));
    TEST_ASSERT_EQUAL_HEX(0x00FF, concat_u16(0x00, 0xFF));
}

void test_set_bits(void) {
    uint8_t bits = 0;

    set_bits(&bits, 0x23, true);
    TEST_ASSERT_EQUAL_HEX(0x23, bits);
    set_bits(&bits, 0x21, false);
    TEST_ASSERT_EQUAL_HEX(0x02, bits);
    set_bits(&bits, 0x8C, true);
    TEST_ASSERT_EQUAL_HEX(0x8E, bits);
    set_bits(&bits, 0xFF, false);
    TEST_ASSERT_EQUAL_HEX(0x00, bits);
    set_bits(&bits, 0xFF, true);
    TEST_ASSERT_EQUAL_HEX(0xFF, bits);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_concat_u16);
    RUN_TEST(test_set_bits);
    return UNITY_END();
}
