// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h" // IWYU pragma: keep

#include "c_pal/srw_lock.h"

static const char* TEST_LOCK_NAME = "test_lock";

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(srw_lock_create_succeeds)
{
    ///act
    SRW_LOCK_HANDLE lock_handle = srw_lock_create(false, TEST_LOCK_NAME);

    srw_lock_acquire_exclusive(lock_handle);

    ///assert
    ASSERT_ARE_EQUAL(int, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_try_acquire_exclusive(lock_handle));

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
