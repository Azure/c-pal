// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h" // IWYU pragma: keep

#include "c_pal/srw_lock_ll.h"

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

TEST_FUNCTION(srw_lock_ll_exclusive_lock_succeeds)
{
    ///act
    SRW_LOCK_LL lock;

    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));

    srw_lock_ll_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_ll_try_acquire_exclusive(&lock));

    srw_lock_ll_release_exclusive(&lock);

    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_OK, srw_lock_ll_try_acquire_exclusive(&lock));

    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_ll_try_acquire_shared(&lock));

    srw_lock_ll_release_exclusive(&lock);

    ///clean
    srw_lock_ll_deinit(&lock);
}

TEST_FUNCTION(srw_lock_ll_shared_lock_succeeds)
{
    ///act
    SRW_LOCK_LL lock;

    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));

    srw_lock_ll_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_ll_try_acquire_exclusive(&lock));

    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_OK, srw_lock_ll_try_acquire_shared(&lock));

    ///assert
    srw_lock_ll_release_shared(&lock);

    ASSERT_ARE_EQUAL(int, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, srw_lock_ll_try_acquire_exclusive(&lock));

    srw_lock_ll_release_shared(&lock);

    ///clean
    srw_lock_ll_deinit(&lock);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
