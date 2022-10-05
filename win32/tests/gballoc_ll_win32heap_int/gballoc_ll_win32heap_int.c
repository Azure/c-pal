// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "c_pal/gballoc_ll.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_init(NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_ll_deinit();

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
    ///arrange
    gballoc_ll_deinit();

    ///act
    gballoc_ll_init(NULL);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_deinit_works)
{
    ///arrange 

    ///act
    gballoc_ll_deinit();

    ///assert - doesn't crash

    ///clean
    (void)gballoc_ll_init(NULL); /*leave it as found - that is in "init state"*/
}

TEST_FUNCTION(gballoc_ll_malloc_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc(1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_1MB_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc(1024 * 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1024*1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc_2(1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_1MB_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc_2(1024 , 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc_flex(1, 1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 2); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_1MB_works)
{
    ///act (1)
    unsigned char* ptr = (unsigned char*)gballoc_ll_malloc_flex(1024, 1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, '3', 1024 + 1024 * 1024); /*can be written*/

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

TEST_FUNCTION(gballoc_ll_realloc_2_works)
{
    ///arrange
    unsigned char* ptr1 = (unsigned char*)gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = (unsigned char*)gballoc_ll_realloc_2(ptr1, 2, 1);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_realloc_flex_works)
{
    ///arrange
    unsigned char* ptr1 = (unsigned char*)gballoc_ll_malloc_flex(4, 12, 34);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = (unsigned char*)gballoc_ll_realloc_flex(ptr1, 4, 13, 45);

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
    void* ptr = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    size_t size;

    ///act
    size = gballoc_ll_size(ptr);

    ///assert
    ASSERT_ARE_EQUAL(size_t, 1, size);

    ///clean
    gballoc_ll_free(ptr);

}

TEST_FUNCTION(gballoc_ll_print_stats_can_be_called_but_does_nothing)
{
    /// arrange

    ///act
    gballoc_ll_print_stats();

    ///assert
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
