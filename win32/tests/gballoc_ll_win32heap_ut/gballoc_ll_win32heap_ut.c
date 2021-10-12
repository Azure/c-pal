// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "windows.h"

static TEST_MUTEX_HANDLE g_testByTest;

static void* TEST_MALLOC_RESULT = (void*)0x1;
static void* TEST_REALLOC_RESULT = (void*)0x3;
static HANDLE TEST_HEAP = (HANDLE)0x4;

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif
    MOCKABLE_FUNCTION(, void*, mock_HeapCreate, DWORD, flOptions, SIZE_T, dwInitialSize, SIZE_T, dwMaximumSize);
    MOCKABLE_FUNCTION(, void*, mock_HeapDestroy, HANDLE, hHeap);
    MOCKABLE_FUNCTION(, void*, mock_HeapAlloc, HANDLE, hHeap, DWORD, dwFlags, SIZE_T, dwBytes);
    MOCKABLE_FUNCTION(, void, mock_HeapFree, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem);
    MOCKABLE_FUNCTION(, void*, mock_HeapReAlloc, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem, SIZE_T, dwBytes);
    MOCKABLE_FUNCTION(, size_t, mock_HeapSize, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem);
#ifdef __cplusplus
}
#endif

#include "c_pal/lazy_init.h"
#undef ENABLE_MOCKS

static LAZY_INIT_RESULT my_lazy_init(call_once_t* lazy, LAZY_INIT_FUNCTION do_init, void* init_params)
{
    if (*lazy == 0)
    {
        if (do_init(init_params) == 0)
        {
            *lazy = 2;
            return LAZY_INIT_OK;
        }
        else
        {
            *lazy = 0;
            return LAZY_INIT_ERROR;
        }
    }
    else
    {
        return LAZY_INIT_OK;
    }
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#include "real_interlocked.h"

#include "c_pal/gballoc_ll.h"

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_RESULT);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(LAZY_INIT_RESULT, LAZY_INIT_RESULT_RESULT);

static void TEST_gballoc_ll_init(void)
{
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_init(NULL));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());

    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapCreate, TEST_HEAP);
    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapAlloc, TEST_MALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapReAlloc, TEST_REALLOC_RESULT);

    REGISTER_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(SIZE_T, size_t);
    REGISTER_UMOCK_ALIAS_TYPE(LAZY_INIT_FUNCTION, size_t);
    REGISTER_GLOBAL_MOCK_HOOK(lazy_init, my_lazy_init);

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_018: [ gballoc_ll_init shall call lazy_init with parameter do_init set to heap_init. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_019: [ heap_init shall call HeapCreate(0,0,0) to create a heap. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_020: [ heap_init shall succeed and return 0. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_002: [ gballoc_ll_init shall succeed and return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_succeeds)
{
    ///arrange
    LAZY_INIT_FUNCTION do_init;
    
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_do_init(&do_init);

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    ///act
    int result = gballoc_ll_init(NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}


/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_002: [ gballoc_ll_init shall succeed and return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_with_non_NULL_pointer_returns_0)
{
    ///arrange
    int result;
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    ///act
    result = gballoc_ll_init((void*)0x24);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_003: [ If there are any failures then gballoc_ll_init shall fail and return a non-0 value. ]*/
TEST_FUNCTION(gballoc_ll_init_unhappy)
{
    ///arrange
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    int result = gballoc_ll_init(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_021: [ If there are any failures then heap_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_init_fails_when_HeapCreate_fails)
{
    ///arrange
    int result;
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .SetReturn(NULL);

    ///act
    result = gballoc_ll_init((void*)0x24);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_016: [ If the global state is not initialized then gballoc_ll_deinit shall return. ]*/
TEST_FUNCTION(gballoc_ll_deinit_without_init)
{
    ///arrange

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_004: [ gballoc_ll_deinit shall call HeapDestroy on the handle stored by gballoc_ll_init in the global variable. ]*/
TEST_FUNCTION(gballoc_ll_deinit_success)
{
    ///arrange
    TEST_gballoc_ll_init();


    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));

    STRICT_EXPECTED_CALL(mock_HeapDestroy(TEST_HEAP));

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_022: [ gballoc_ll_malloc shall call lazy_init with parameter do_init set to heap_init. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_006: [ gballoc_ll_malloc shall call HeapAlloc. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_007: [ gballoc_ll_malloc shall return what HeapAlloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_succeeds)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, 0, 1));

    ///act
    void* malloc_result = gballoc_ll_malloc(1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_023: [ If lazy_init fails then gballoc_ll_mallocshall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_fails_when_lazy_init_fails)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    void* malloc_result = gballoc_ll_malloc(1);

    ///assert
    ASSERT_IS_NULL(malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_028: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_overflow_fails)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_malloc_2(2, SIZE_MAX / 2 + 1); /*a clear overflow*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_029: [ gballoc_ll_malloc_2 shall call lazy_init with parameter do_init set to heap_init. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_031: [ gballoc_ll_malloc_2 shall call HeapAlloc(nmemb*size) and return what HeapAlloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_succeeds)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, 0, 1 * 2));

    ///act
    void* malloc_result = gballoc_ll_malloc_2(3, SIZE_MAX/3);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_030: [ If lazy_init fails then gballoc_ll_malloc_2 shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_fails_when_lazy_init_fails)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    void* malloc_result = gballoc_ll_malloc_2(1, 2);

    ///assert
    ASSERT_IS_NULL(malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}


/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_009: [ gballoc_ll_free shall call HeapFree. ]*/
TEST_FUNCTION(gballoc_ll_free_success)
{
    ///arrange
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();
    void* what = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(what);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapFree(TEST_HEAP, 0, what));

    ///act
    gballoc_ll_free(what);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_024: [ gballoc_ll_calloc shall call lazy_init with parameter do_init set to heap_init. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_011: [ gballoc_ll_calloc shall call HeapAlloc with flags set to HEAP_ZERO_MEMORY. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_012: [ gballoc_ll_calloc shall return what HeapAlloc returns. ]*/
TEST_FUNCTION(gballoc_ll_calloc_succeeds)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, HEAP_ZERO_MEMORY, 2));

    ///act
    void* malloc_result = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_025: [ If lazy_init fails then gballoc_ll_calloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_calloc_fails_when_lazy_init_fails)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    void* malloc_result = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_IS_NULL(malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_026: [ gballoc_ll_realloc shall call lazy_init with parameter do_init set to heap_init. ]*/
TEST_FUNCTION(gballoc_ll_realloc_without_init)
{
    ///arrange

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    STRICT_EXPECTED_CALL(mock_HeapReAlloc(TEST_HEAP, 0, (void*)3, 1));

    ///act
    void* ptr = gballoc_ll_realloc((void*)3, 1);

    ///assert
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_027: [ If lazy_init fails then gballoc_ll_realloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_fails_when_lazy_init_fails)
{
    ///arrange

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    void* ptr = gballoc_ll_realloc((void*)3, 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}


/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_014: [ If ptr is NULL then gballoc_ll_realloc shall call HeapAlloc and return what HeapAlloc returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_with_ptr_NULL)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, 0, 1));

    ///act
    void* ptr = gballoc_ll_realloc(NULL, 1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_015: [ If ptr is not NULL then gballoc_ll_realloc shall call HeapReAlloc and return what HeapReAlloc returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_with_ptr_non_NULL)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mock_HeapReAlloc(TEST_HEAP, 0, TEST_MALLOC_RESULT, 1));

    ///act
    void* ptr = gballoc_ll_realloc(TEST_MALLOC_RESULT, 1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_017: [ gballoc_ll_size shall call HeapSize and return what HeapSize returns. ]*/
TEST_FUNCTION(gballoc_ll_size_returns_what_HeapSize_returned)
{
    ///arrange
    TEST_gballoc_ll_init();

    STRICT_EXPECTED_CALL(mock_HeapSize(TEST_HEAP, 0, TEST_MALLOC_RESULT))
        .SetReturn(1);

    ///act
    size_t size = gballoc_ll_size(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(size_t, 1, size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_017: [ gballoc_ll_size shall call HeapSize and return what HeapSize returns. ]*/
TEST_FUNCTION(gballoc_ll_size_returns_what_HeapSize_returned_when_it_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_HeapSize(IGNORED_ARG, 0, TEST_MALLOC_RESULT))
        .SetReturn((SIZE_T)(-1));

    ///act
    size_t size = gballoc_ll_size(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(size_t, (size_t)(-1), size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
