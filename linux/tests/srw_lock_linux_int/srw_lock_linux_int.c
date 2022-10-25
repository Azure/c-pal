// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdarg.h>              // for va_end, va_list, va_start
#include <stdlib.h>

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_pal/srw_lock.h"

static const char* TEST_LOCK_NAME = "test_lock";

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

TEST_FUNCTION(srw_lock_exclusive_lock)
{
    ///act
    SRW_LOCK_HANDLE lock_handle = srw_lock_create(false, TEST_LOCK_NAME);

    srw_lock_acquire_exclusive(lock_handle);

    ///assert
    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_try_acquire_exclusive(lock_handle) );

    srw_lock_release_exclusive(lock_handle);

    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_OK, srw_lock_try_acquire_exclusive(lock_handle));

    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_try_acquire_shared(lock_handle));

    srw_lock_release_exclusive(lock_handle);

    ///clean
    srw_lock_destroy(lock_handle);
}

TEST_FUNCTION(srw_lock_shared_lock)
{
    ///act
    SRW_LOCK_HANDLE lock_handle = srw_lock_create(false, TEST_LOCK_NAME);

    srw_lock_acquire_shared(lock_handle);

    ///assert
    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_try_acquire_exclusive(lock_handle));

    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_OK, srw_lock_try_acquire_shared(lock_handle));

    ///assert
    srw_lock_release_shared(lock_handle);

    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_try_acquire_exclusive(lock_handle));

    srw_lock_release_shared(lock_handle);

    ///clean
    srw_lock_destroy(lock_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
