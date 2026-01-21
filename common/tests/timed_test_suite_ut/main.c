// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>

#include "c_logging/logger.h"

#include "ctest.h"

#include "timed_test_suite_ut.h"

int main(void)
{
    size_t failedTests = 0;

    (void)logger_init();

    CTEST_RUN_TEST_SUITE(timed_test_suite_ut);
    if (timed_test_suite_ut_succeeded() != 0)
    {
        // Final validation of fixture ordering has failed
        failedTests++;
    }

    logger_deinit();

    return (int)failedTests;
}
