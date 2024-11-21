// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_ll.h"

#include "jemalloc/jemalloc.h"

#define DECAY_MS 50000
#define MAX_ARENAS 5

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
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
    unsigned char* ptr = gballoc_ll_malloc(1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc(1024 * 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_2(1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_2(1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_flex(1, 1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1 + 1 * 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_1MB_works)
{
    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_flex(1024, 1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 + 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_free_works)
{
    ///arrange
    unsigned char* ptr = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    ///act 
    gballoc_ll_free(ptr);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_realloc_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc(ptr1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_realloc_2_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc_2(ptr1, 1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_realloc_flex_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc_flex(4, 10, 8);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc_flex(ptr1, 4, 20, 8);

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
    ptr = gballoc_ll_calloc(1, 1);

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
    ASSERT_IS_TRUE(size>=4);

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_print_stats_works)
{
    /// arrange
    void* ptr = gballoc_ll_malloc(4);
    ASSERT_IS_NOT_NULL(ptr);

    ///act
    gballoc_ll_print_stats();

    ///assert - this is less than ideal, but the original size asked to be malloc'd is lost

    ///clean
    gballoc_ll_free(ptr);
}

static void assert_set_option_works_for_decay(const char* option_name, int64_t old_decay_ms, int64_t decay_ms, uint32_t narenas)
{
    char command[32];
    int snprintf_result;
    int64_t verify_decay_ms;
    size_t decay_ms_size = sizeof(decay_ms);

    // Arena 1 does not exist, so je_mallctl should return EFAULT
    // The last arena (narenas - 1) is reserved for huge allocations, hence we cannot set decay for it
    for (uint32_t i = 0; i < narenas; i++)
    {
        snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu32 ".%s_ms", i, option_name);
        ASSERT_IS_TRUE(snprintf_result > 0 && snprintf_result < sizeof(command), "snprintf failed");

        if (i == 1)
        {
            // the arena at index 1 does not exist, so this should fail with EFAULT
            ASSERT_ARE_EQUAL(int, EFAULT, je_mallctl(command, &verify_decay_ms, &decay_ms_size, NULL, 0));
        }
        else if(i == narenas - 1)
        {
            // the last arena is reserved for huge arena, so this return the old decay value
            ASSERT_ARE_EQUAL(int, 0, je_mallctl(command, &verify_decay_ms, &decay_ms_size, NULL, 0));
            ASSERT_ARE_EQUAL(int64_t, old_decay_ms, verify_decay_ms);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, je_mallctl(command, &verify_decay_ms, &decay_ms_size, NULL, 0));
            ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);
        }
    }
}

static void setup_arenas_for_set_option(uint32_t expected_narenas, void* ptr[])
{
    // verify the number of arenas
    uint32_t narenas;
    size_t narenas_size = sizeof(narenas);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.narenas", &narenas, &narenas_size, NULL, 0));
    ASSERT_ARE_EQUAL(uint32_t, expected_narenas, narenas);

    ptr[0] = gballoc_ll_malloc(4); // this malloc will happen in the default arena since this thread is linked to arena 0 by default
    ASSERT_IS_NOT_NULL(ptr[0]);

    // Initialize all the arenas except first(arena at index 1) after the default arena, so that we can verify that the decay is set for all the arenas except arena at index 1 and the last arena(reserved for huge arena)
    uint32_t arena_id;
    for (uint32_t i = 2; i < narenas; i++)
    {
        arena_id = i; // assign thread to arena i
        ASSERT_ARE_EQUAL(int, 0, je_mallctl("thread.arena", NULL, NULL, &arena_id, sizeof(arena_id)));

        ptr[i] = gballoc_ll_malloc(4); // this malloc will happen in the arena i
        ASSERT_IS_NOT_NULL(ptr[i]);
    }
}

TEST_FUNCTION(gballoc_ll_set_option_works_for_dirty_decay)
{
    /// arrange
    // configure the max number of arenas to 4 (hence total max limit for number of arenas = 5, 1 default + 4 configured)
    je_malloc_conf = "narenas:4";

    uint32_t expected_narenas = MAX_ARENAS;
    void* ptr[MAX_ARENAS] = { NULL };

    setup_arenas_for_set_option(expected_narenas, ptr);

    int result;
    int64_t decay_ms = DECAY_MS;
    size_t decay_ms_size = sizeof(decay_ms);
    int64_t verify_decay_ms;
    int64_t old_decay_ms;

    // Retrieve the old decay value
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.dirty_decay_ms", &old_decay_ms, &decay_ms_size, NULL, 0));

    ///act
    result = gballoc_ll_set_option("dirty_decay", &decay_ms);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.dirty_decay_ms", &verify_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);

    // verify that all the arenas have the decay set except arena 1 and last arena(reserved for huge arena)
    assert_set_option_works_for_decay("dirty_decay", old_decay_ms, decay_ms, expected_narenas);

    ///clean
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
}

TEST_FUNCTION(gballoc_ll_set_option_works_for_muzzy_decay)
{
    /// arrange
    // configure the max number of arenas to 4 (hence total max limit for number of arenas = 5, 1 default + 4 configured)
    je_malloc_conf = "narenas:4";

    uint32_t expected_narenas = MAX_ARENAS;
    void* ptr[MAX_ARENAS] = { NULL };

    setup_arenas_for_set_option(expected_narenas, ptr);

    int result;
    int64_t decay_ms = DECAY_MS;
    size_t decay_ms_size = sizeof(decay_ms);
    int64_t verify_decay_ms;
    int64_t old_decay_ms;

    // Retrieve the old decay value
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.muzzy_decay_ms", &old_decay_ms, &decay_ms_size, NULL, 0));

    ///act
    result = gballoc_ll_set_option("muzzy_decay", &decay_ms);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.muzzy_decay_ms", &verify_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);

    // verify that all the arenas have the decay set except arena 1 and last arena(reserved for huge arena)
    assert_set_option_works_for_decay("muzzy_decay", old_decay_ms, decay_ms, expected_narenas);

    ///clean
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
