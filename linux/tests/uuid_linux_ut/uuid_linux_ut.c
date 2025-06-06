// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, mocked_uuid_generate, uuid_t, out);
#undef ENABLE_MOCKS

#include "c_pal/uuid.h"

#define TEST_DATA_0  0x01
#define TEST_DATA_1  0x12
#define TEST_DATA_2  0x23
#define TEST_DATA_3  0x34
#define TEST_DATA_4  0x45
#define TEST_DATA_5  0x56
#define TEST_DATA_6  0x67
#define TEST_DATA_7  0x78
#define TEST_DATA_8  0x89
#define TEST_DATA_9  0x9A
#define TEST_DATA_10 0xAB
#define TEST_DATA_11 0xBC
#define TEST_DATA_12 0xCD
#define TEST_DATA_13 0xDE
#define TEST_DATA_14 0xEF
#define TEST_DATA_15 0xF0

static void hook_uuid_generate(uuid_t out)
{
    out[0]  = TEST_DATA_0 ;
    out[1]  = TEST_DATA_1 ;
    out[2]  = TEST_DATA_2 ;
    out[3]  = TEST_DATA_3 ;
    out[4]  = TEST_DATA_4 ;
    out[5]  = TEST_DATA_5 ;
    out[6]  = TEST_DATA_6 ;
    out[7]  = TEST_DATA_7 ;
    out[8]  = TEST_DATA_8 ;
    out[9]  = TEST_DATA_9 ;
    out[10] = TEST_DATA_10;
    out[11] = TEST_DATA_11;
    out[12] = TEST_DATA_12;
    out[13] = TEST_DATA_13;
    out[14] = TEST_DATA_14;
    out[15] = TEST_DATA_15;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static char* umock_stringify_uuid_t(const uuid_t* value)
{
    char* result;
    result = malloc(37);
    ASSERT_IS_NOT_NULL(result);
    (void)sprintf(result, "%" PRI_UUID_T "", UUID_T_VALUES(*value));
    return result;
}

static int umock_are_equal_uuid_t(const uuid_t* left, const uuid_t* right)
{
    int result;
    if (left == NULL)
    {
        if (right == NULL)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        if (right == NULL)
        {
            result = 0;
        }
        else
        {
            result = (memcmp(*left, *right, sizeof(uuid_t)) == 0);
        }
    }
    return result;
}

static int umock_copy_uuid_t(uuid_t* destination, const uuid_t* source)
{
    if (
        (destination == NULL) || 
        (source == NULL)
        )
    {
        return 1;
    }
    else
    {
        (void)memcpy(*destination, source, sizeof(uuid_t));
        return 0;
    }
}

static void umock_free_uuid_t(uuid_t* value)
{
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_UMOCK_VALUE_TYPE(uuid_t);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_generate, hook_uuid_generate);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_UUID_LINUX_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_produce_with_destination_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = uuid_produce(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

}

/*Tests_SRS_UUID_LINUX_02_002: [ uuid_produce shall call uuid_generate to generate a UUID. ]*/
/*Tests_SRS_UUID_LINUX_02_004: [ uuid_produce shall succeed and return 0. ]*/
TEST_FUNCTION(uuid_produce_succeeds)
{
    ///arrange
    int result;
    UUID_T u = { 0 };

    STRICT_EXPECTED_CALL(mocked_uuid_generate(IGNORED_ARG));

    ///act
    result = uuid_produce(u);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_0,  u[0]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_1,  u[1]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_2,  u[2]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_3,  u[3]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4,  u[4]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_5,  u[5]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_6,  u[6]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_7,  u[7]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_8,  u[8]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_9,  u[9]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_10, u[10]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_11, u[11]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_12, u[12]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_13, u[13]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_14, u[14]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_15, u[15]);
}

// is_uuid_nil

// Tests_SRS_UUID_LINUX_11_001: [ if uuid_value is NULL then is_uuid_nil shall fail and return true. ]
TEST_FUNCTION(is_uuid_nil_uuid_is_NULL)
{
    ///arrange

    ///act
    bool result = is_uuid_nil(NULL);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_UUID_LINUX_11_002: [ If all the values of is_uuid_nil are 0 then is_uuid_nil shall return true. ]
TEST_FUNCTION(is_uuid_nil_on_valid_uuid)
{
    ///arrange
    UUID_T valid_uuid;
    hook_uuid_generate(valid_uuid);

    ///act
    bool result = is_uuid_nil(valid_uuid);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_UUID_LINUX_11_003: [ If any the values of is_uuid_nil are not 0 then is_uuid_nil shall return false. ]
TEST_FUNCTION(is_uuid_nil_on_nil_uuid)
{
    ///arrange
    UUID_T valid_uuid = { 0 };

    ///act
    bool result = is_uuid_nil(valid_uuid);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
