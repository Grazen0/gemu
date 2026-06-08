#include "log.h"
#include <unity.h>

void test_log_level_from_str_trace()
{
    LogLevel level = -1;
    TEST_ASSERT_TRUE(log_level_from_str("trace", &level));
    TEST_ASSERT_EQUAL(LOG_LEVEL_TRACE, level);
}

void test_log_level_from_str_debug()
{
    LogLevel level = -1;
    TEST_ASSERT_TRUE(log_level_from_str("debug", &level));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, level);
}

void test_log_level_from_str_info()
{
    LogLevel level = -1;
    TEST_ASSERT_TRUE(log_level_from_str("info", &level));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, level);
}

void test_log_level_from_str_warn()
{
    LogLevel level = -1;
    TEST_ASSERT_TRUE(log_level_from_str("warn", &level));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, level);
}

void test_log_level_from_str_error()
{
    LogLevel level = -1;
    TEST_ASSERT_TRUE(log_level_from_str("error", &level));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, level);
}

void test_log_level_from_str_invalid()
{
    LogLevel level = -1;
    TEST_ASSERT_FALSE(log_level_from_str("invalid", &level));
}

void test_log_level_from_str_empty()
{
    LogLevel level = -1;
    TEST_ASSERT_FALSE(log_level_from_str("", &level));
}

void test_log_level_from_str_case_sensitive()
{
    LogLevel level = -1;
    TEST_ASSERT_FALSE(log_level_from_str("DEBUG", &level));
}
