// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "azure_macro_utils/macro_utils.h"
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
    MOCKABLE_FUNCTION(, void*, mock_HeapFree, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem);
    MOCKABLE_FUNCTION(, void*, mock_HeapReAlloc, HANDLE, hHeap, DWORD, dwFlags, LPVOID, lpMem, SIZE_T, dwBytes);
#ifdef __cplusplus
}
#endif
#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#include "azure_c_pal/gballoc_ll.h"

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(gballoc_ll_win32heap_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());

    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapCreate, TEST_HEAP);
    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapAlloc, TEST_MALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_HeapReAlloc, TEST_REALLOC_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(SIZE_T, size_t);

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

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_001: [ gballoc_ll_init shall call HeapCreate(0,0,0) and store the returned heap handle in a global variable. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_002: [ gballoc_ll_init shall succeed and return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    ///act
    int result = gballoc_ll_init(NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_001: [ gballoc_ll_init shall call HeapCreate(0,0,0) and store the returned heap handle in a global variable. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_002: [ gballoc_ll_init shall succeed and return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_with_non_NULL_pointer_returns_0)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0));

    ///act
    result = gballoc_ll_init((void*)0x24);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_003: [ If HeapCreate fails then gballoc_ll_init shall fail and return a non-0 value. ]*/
TEST_FUNCTION(gballoc_ll_init_unhappy)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_HeapCreate(0, 0, 0))
        .SetReturn(NULL);

    ///act
    int result = gballoc_ll_init(NULL);

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
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapDestroy(TEST_HEAP));

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_005: [ If the global state is not initialized then gballoc_ll_malloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_without_init_fails)
{
    ///arrange

    ///act
    void* result = gballoc_ll_malloc(1);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_006: [ gballoc_ll_malloc shall call HeapAlloc. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_007: [ gballoc_ll_malloc shall return what HeapAlloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_succeeds)
{
    ///arrange
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, 0, 1));

    ///act
    void* malloc_result = gballoc_ll_malloc(1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_008: [ If the global state is not initialized then gballoc_ll_free shall return. ]*/
TEST_FUNCTION(gballoc_ll_free_without_init_returns)
{
    ///arrange

    ///act
    gballoc_ll_free(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
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

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_010: [ If the global state is not initialized then gballoc_ll_calloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_calloc_without_init_returns_NULL)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_calloc(1, 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_011: [ gballoc_ll_calloc shall call HeapAlloc with flags set to HEAP_ZERO_MEMORY. ]*/
/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_012: [ gballoc_ll_calloc shall return what HeapAlloc returns. ]*/
TEST_FUNCTION(gballoc_ll_calloc_succeeds)
{
    ///arrange
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapAlloc(TEST_HEAP, HEAP_ZERO_MEMORY, 2));

    ///act
    void* malloc_result = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, malloc_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_013: [ If the global state is not initialized then gballoc_ll_realloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_without_init_returns_NULL)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_realloc((void*)3, 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_WIN32HEAP_02_014: [ If ptr is NULL then gballoc_ll_realloc shall call HeapAlloc and return what HeapAlloc returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_with_ptr_NULL)
{
    ///arrange
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

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
    int result = gballoc_ll_init(NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mock_HeapReAlloc(TEST_HEAP, 0, TEST_MALLOC_RESULT, 1));

    ///act
    void* ptr = gballoc_ll_realloc(TEST_MALLOC_RESULT, 1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

END_TEST_SUITE(gballoc_ll_win32heap_ut)
