// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "lazy_init_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, int, do_init, void*, params);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)


static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT);
    REGISTER_TYPE(LAZY_INIT_RESULT, CALL_ONCE_RESULT);

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_LAZY_INIT_02_001: [ If lazy is NULL then lazy_init shall fail and return LAZY_INIT_ERROR. ]*/
TEST_FUNCTION(lazy_init_with_lazy_NULL_fails)
{
    ///arrange
    LAZY_INIT_RESULT result;

    ///act
    result = lazy_init(NULL, do_init, NULL);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_ERROR, result);

    ///clean
}

/*Tests_SRS_LAZY_INIT_02_002: [ If do_init is NULL then lazy_init shall fail and return LAZY_INIT_ERROR. ]*/
TEST_FUNCTION(lazy_init_with_do_init_NULL_fails)
{
    ///arrange
    LAZY_INIT_RESULT result;
    call_once_t lazy = LAZY_INIT_NOT_DONE;

    ///act
    result = lazy_init(&lazy, NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_ERROR, result);

    ///clean
}

/*Tests_SRS_LAZY_INIT_02_003: [ lazy_init shall call call_once_begin(lazy). ]*/
/*Tests_SRS_LAZY_INIT_02_005: [ If call_once_begin returns CALL_ONCE_PROCEED then lazy_init shall call do_init(init_params). ]*/
/*Tests_SRS_LAZY_INIT_02_006: [ If do_init returns 0 then lazy_init shall call call_once_end(lazy, true), succeed and return LAZY_INIT_OK. ]*/
TEST_FUNCTION(lazy_init_inits_once_with_success)
{
    ///arrange
    LAZY_INIT_RESULT result;
    volatile_atomic int32_t lazy = LAZY_INIT_NOT_DONE;

    STRICT_EXPECTED_CALL(call_once_begin(&lazy))
        .SetReturn(CALL_ONCE_PROCEED);
    STRICT_EXPECTED_CALL(do_init((void*)3))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(call_once_end(&lazy, true));

    ///act
    result = lazy_init(&lazy, do_init, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_OK, result);

    ///clean
}

/*Tests_SRS_LAZY_INIT_02_003: [ lazy_init shall call call_once_begin(lazy). ]*/
/*Tests_SRS_LAZY_INIT_02_005: [ If call_once_begin returns CALL_ONCE_PROCEED then lazy_init shall call do_init(init_params). ]*/
/*Tests_SRS_LAZY_INIT_02_007: [ If do_init returns different than 0 then lazy_init shall call call_once_end(lazy, false), fail and return LAZY_INIT_ERROR. ]*/
TEST_FUNCTION(lazy_init_inits_once_with_failure)
{
    ///arrange
    LAZY_INIT_RESULT result;
    volatile_atomic int32_t lazy = LAZY_INIT_NOT_DONE;

    STRICT_EXPECTED_CALL(call_once_begin(&lazy))
        .SetReturn(CALL_ONCE_PROCEED);
    STRICT_EXPECTED_CALL(do_init((void*)3))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(call_once_end(&lazy, true));

    ///act
    result = lazy_init(&lazy, do_init, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_ERROR, result);

    ///clean
}

/*Tests_SRS_LAZY_INIT_02_004: [ If call_once_begin returns CALL_ONCE_ALREADY_CALLED then lazy_init shall succeed and return LAZY_INIT_OK. ]*/
TEST_FUNCTION(lazy_init_does_not_init_twice)
{
    ///arrange
    LAZY_INIT_RESULT result;
    volatile_atomic int32_t lazy = LAZY_INIT_NOT_DONE;

    STRICT_EXPECTED_CALL(call_once_begin(&lazy))
        .SetReturn(CALL_ONCE_ALREADY_CALLED);

    ///act
    result = lazy_init(&lazy, do_init, (void*)3);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_OK, result);

    ///clean
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
