// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/thandle_ll.h"           // for THANDLE, THANDLE_ASSIGN

#define ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle_ptr.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}


typedef struct UNDER_TEST_TAG
{
    int a;
}UNDER_TEST;

typedef UNDER_TEST* UNDER_TEST_PTR;

THANDLE_PTR_DECLARE(UNDER_TEST_PTR);
THANDLE_PTR_DEFINE(UNDER_TEST_PTR);


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    umock_c_init(on_umock_c_error);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

static void just_call_free(UNDER_TEST_PTR p)
{
    free(p);
}

/*Tests_SRS_THANDLE_PTR_02_001: [ THANDLE_PTR_CREATE_WITH_MOVE(T) shall return what THANDLE_CREATE_FROM_CONTENT(PTR(T))(THANDLE_PTR_DISPOSE(T)) returns. ]*/
TEST_FUNCTION(THANDLE_PTR_CREATE_WITH_MOVE_happy_path)
{
    ///arrange
    UNDER_TEST_PTR p = real_gballoc_hl_malloc(sizeof(UNDER_TEST));
    ASSERT_IS_NOT_NULL(p);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*this is THANDLE_CREATE_FROM_CONTENT...*/

    ///act
    THANDLE(PTR(UNDER_TEST_PTR)) t = THANDLE_PTR_CREATE_WITH_MOVE(UNDER_TEST_PTR)(p, just_call_free);

    ///assert
    ASSERT_IS_NOT_NULL(t);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(PTR(UNDER_TEST_PTR))(&t, NULL);

}

/*Tests_SRS_THANDLE_PTR_02_001: [ THANDLE_PTR_CREATE_WITH_MOVE(T) shall return what THANDLE_CREATE_FROM_CONTENT(PTR(T))(THANDLE_PTR_DISPOSE(T)) returns. ]*/
TEST_FUNCTION(THANDLE_PTR_CREATE_WITH_MOVE_unhappy_path)
{
    ///arrange
    UNDER_TEST_PTR p = real_gballoc_hl_malloc(sizeof(UNDER_TEST));
    ASSERT_IS_NOT_NULL(p);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)) /*this is THANDLE_CREATE_FROM_CONTENT...*/
        .SetReturn(NULL);

    ///act
    THANDLE(PTR(UNDER_TEST_PTR)) t = THANDLE_PTR_CREATE_WITH_MOVE(UNDER_TEST_PTR)(p, just_call_free);

    ///assert
    ASSERT_IS_NULL(t);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    just_call_free(p);
}

/*Tests_SRS_THANDLE_PTR_02_002: [ If the original dispose is non-NULL then THANDLE_PTR_DISPOSE(T) shall call dispose. ]*/
TEST_FUNCTION(THANDLE_PTR_DISPOSE_call_dispose)
{
    ///arrange
    UNDER_TEST_PTR p = real_gballoc_hl_malloc(sizeof(UNDER_TEST));
    ASSERT_IS_NOT_NULL(p);

    THANDLE(PTR(UNDER_TEST_PTR)) t = THANDLE_PTR_CREATE_WITH_MOVE(UNDER_TEST_PTR)(p, just_call_free);
    ASSERT_IS_NOT_NULL(t);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(p));           /*this is freeing "p"*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is freeing the THANDLE storage space*/

    ///act
    THANDLE_ASSIGN(PTR(UNDER_TEST_PTR))(&t, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls()); /*this is "shall return"... */
}

/*Tests_SRS_THANDLE_PTR_02_003: [ THANDLE_PTR_DISPOSE(T) shall return. ]*/
TEST_FUNCTION(THANDLE_PTR_DISPOSE_NULL_does_not_call_anything)
{
    ///arrange
    UNDER_TEST local = { .a = 42 };
    UNDER_TEST_PTR p = &local; /*note: no function needed to be called to dispose of local*/
    ASSERT_IS_NOT_NULL(p);

    THANDLE(PTR(UNDER_TEST_PTR)) t = THANDLE_PTR_CREATE_WITH_MOVE(UNDER_TEST_PTR)(p, NULL);
    ASSERT_IS_NOT_NULL(t);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is freeing the THANDLE storage space*/

    ///act
    THANDLE_ASSIGN(PTR(UNDER_TEST_PTR))(&t, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls()); /*this is "shall return"... */
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

