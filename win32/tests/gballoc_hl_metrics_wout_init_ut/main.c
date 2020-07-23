// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>
#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(gballoc_hl_metrics_wout_init_unittests, failedTestCount);
    return failedTestCount;
}
