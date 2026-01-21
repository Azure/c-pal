// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <stdio.h>

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTests = 0;

    if (logger_init() != 0)
    {
        (void)printf("logger_init failed\r\n");
        failedTests++;
    }
    else
    {
        RUN_TEST_SUITE(timed_test_suite_ut);

        logger_deinit();
    }

    return (int)failedTests;
}
