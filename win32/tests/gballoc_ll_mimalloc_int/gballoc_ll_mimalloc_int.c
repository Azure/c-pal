// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "c_pal/gballoc_ll.h"

#include "mimalloc.h"

BEGIN_TEST_SUITE(gballoc_ll_mimalloc_int)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


TEST_FUNCTION(gballoc_ll_init_works)
{
    ///act
    gballoc_ll_init(NULL);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_deinit_works)
{
    ///act
    gballoc_ll_deinit();

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_malloc_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc(1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    ptr[0] = '3'; /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_1MB_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc(1024*1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    ptr[0] = '3'; /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_free_works)
{
    ///arrange
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    ///act 
    gballoc_ll_free(ptr);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_realloc_works)
{
    ///arrange
    unsigned char* ptr1 = (unsigned char*)gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = (unsigned char*)gballoc_ll_realloc(ptr1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_calloc_works)
{
    ///arrange
    unsigned char* ptr;

    ///act 
    ptr = (unsigned char*)gballoc_ll_calloc(1, 1);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_TRUE(0 == ptr[0]);

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_size_works)
{
    /// arrange
    void* ptr = gballoc_ll_malloc(4);
    ASSERT_IS_NOT_NULL(ptr);

    size_t size;

    ///act
    size = gballoc_ll_size(ptr);

    ///assert - this is less than ideal, but the original size asked to be malloc'd is lost
    ///see - (mimalloc) https://microsoft.github.io/mimalloc/group__extended.html#ga089c859d9eddc5f9b4bd946cd53cebee - "The returned size is always at least equal to the allocated size of p..."
    ///see - (linux has that too...) https://man7.org/linux/man-pages/man3/malloc_usable_size.3.html - "The value returned by malloc_usable_size() may be greater than the requested size of the allocation[...]"
    ASSERT_IS_TRUE(size>=4);

    ///clean
    gballoc_ll_free(ptr);

}
END_TEST_SUITE(gballoc_ll_mimalloc_int)
