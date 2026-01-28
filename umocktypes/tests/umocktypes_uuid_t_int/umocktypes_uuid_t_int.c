// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "submodule.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/uuid.h"
#include "c_pal/umocktypes_uuid_t.h"

#include "module.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    umock_c_init(on_umock_c_error);

    ASSERT_ARE_EQUAL(int, 0, umocktypes_UUID_T_register_types());
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(module_calls_submodule_1) /*wants to see that the UUID_Ts match*/
{
    // arrange
    UUID_T u = {
        (unsigned char)'a',
        (unsigned char)'b',
        (unsigned char)'c',
        (unsigned char)'d',
        (unsigned char)'e',
        (unsigned char)'f',
        (unsigned char)'g',
        (unsigned char)'h',
        (unsigned char)'i',
        (unsigned char)'j',
        (unsigned char)'k',
        (unsigned char)'l',
        (unsigned char)'m',
        (unsigned char)'n',
        (unsigned char)'o',
        (unsigned char)'p'
    };

    STRICT_EXPECTED_CALL(submodule_reads_UUID_T(u));

    // act
    module_reads_UUID_T(u);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(module_calls_submodule_2) /*wants to see that the UUID_Ts do not match*/
{
    // arrange
    UUID_T u = {
        (unsigned char)'a',
        (unsigned char)'b',
        (unsigned char)'c',
        (unsigned char)'d',
        (unsigned char)'e',
        (unsigned char)'f',
        (unsigned char)'g',
        (unsigned char)'h',
        (unsigned char)'i',
        (unsigned char)'j',
        (unsigned char)'k',
        (unsigned char)'l',
        (unsigned char)'m',
        (unsigned char)'n',
        (unsigned char)'o',
        (unsigned char)'Z' /*note: module calls here with 'p'*/
    };

    STRICT_EXPECTED_CALL(submodule_reads_UUID_T(u));

    // act
    module_reads_UUID_T(u);

    // assert
    ASSERT_ARE_NOT_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(module_calls_const_submodule_1) /*wants to see that the UUID_Ts match, they are const*/
{
    // arrange
    UUID_T u = {
        (unsigned char)'a',
        (unsigned char)'b',
        (unsigned char)'c',
        (unsigned char)'d',
        (unsigned char)'e',
        (unsigned char)'f',
        (unsigned char)'g',
        (unsigned char)'h',
        (unsigned char)'i',
        (unsigned char)'j',
        (unsigned char)'k',
        (unsigned char)'l',
        (unsigned char)'m',
        (unsigned char)'n',
        (unsigned char)'o',
        (unsigned char)'p'
    };

    STRICT_EXPECTED_CALL(submodule_reads_const_UUID_T(u));

    // act
    module_reads_const_UUID_T(u);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(module_calls_const_submodule_2) /*wants to see that the const UUID_Ts do not match*/
{
    // arrange
    UUID_T u = {
        (unsigned char)'a',
        (unsigned char)'b',
        (unsigned char)'c',
        (unsigned char)'d',
        (unsigned char)'e',
        (unsigned char)'f',
        (unsigned char)'g',
        (unsigned char)'h',
        (unsigned char)'i',
        (unsigned char)'j',
        (unsigned char)'k',
        (unsigned char)'l',
        (unsigned char)'m',
        (unsigned char)'n',
        (unsigned char)'o',
        (unsigned char)'Z' /*note: module calls here with 'p'*/
    };

    STRICT_EXPECTED_CALL(submodule_reads_const_UUID_T(u));

    // act
    module_reads_const_UUID_T(u);

    // assert
    ASSERT_ARE_NOT_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
