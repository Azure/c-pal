// Copyright (c) Microsoft. All rights reserved.

#include "real_interlocked.h"
#include "real_thandle_helper.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"

#include "c_pal/threadpool.h"

#include "real_threadpool_thandle.h"

#include "real_interlocked_renames.h"
REAL_THANDLE_DEFINE(THREADPOOL)

static void dispose_REAL_THREADPOOL_do_nothing(REAL_THREADPOOL* nothing)
{
    (void)nothing;
}

THANDLE(THREADPOOL) real_threadpool_thandle_create(void)
{
    THANDLE(THREADPOOL) result = THANDLE_MALLOC(REAL_THREADPOOL)(dispose_REAL_THREADPOOL_do_nothing);
    if (result != NULL)
    {
        THANDLE_GET_T(REAL_THREADPOOL)(result)->dummy = 1;
    }
    return result;
}

