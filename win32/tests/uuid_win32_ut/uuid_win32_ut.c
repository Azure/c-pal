// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "rpc.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, RPC_STATUS, mocked_UuidCreate, UUID*, Uuid);
#undef ENABLE_MOCKS

#include "c_pal/uuid.h"

#define TEST_DATA_1 0x12233445
#define TEST_DATA_2 0x5667
#define TEST_DATA_3 0x7889
#define TEST_DATA_4_0 0x9A
#define TEST_DATA_4_1 0xAB
#define TEST_DATA_4_2 0xBC
#define TEST_DATA_4_3 0xCD
#define TEST_DATA_4_4 0xDE
#define TEST_DATA_4_5 0xEF
#define TEST_DATA_4_6 0xF0
#define TEST_DATA_4_7 0x01

static long g_hook_UuidCreate_return = RPC_S_OK;
static RPC_STATUS hook_UuidCreate(UUID __RPC_FAR* Uuid)
{
    Uuid->Data1 = TEST_DATA_1;
    Uuid->Data2 = TEST_DATA_2;
    Uuid->Data3 = TEST_DATA_3;
    Uuid->Data4[0] = TEST_DATA_4_0;
    Uuid->Data4[1] = TEST_DATA_4_1;
    Uuid->Data4[2] = TEST_DATA_4_2;
    Uuid->Data4[3] = TEST_DATA_4_3;
    Uuid->Data4[4] = TEST_DATA_4_4;
    Uuid->Data4[5] = TEST_DATA_4_5;
    Uuid->Data4[6] = TEST_DATA_4_6;
    Uuid->Data4[7] = TEST_DATA_4_7;
    return g_hook_UuidCreate_return;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_GLOBAL_MOCK_HOOK(mocked_UuidCreate, hook_UuidCreate);

    REGISTER_UMOCK_ALIAS_TYPE(RPC_STATUS, long);

    g_hook_UuidCreate_return = RPC_S_OK;
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

/*Tests_SRS_UUID_WIN32_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_produce_with_destination_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = uuid_produce(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_UUID_WIN32_02_002: [ uuid_produce shall call UuidCreate to generate a UUID. ]*/
/*Tests_SRS_UUID_WIN32_02_003: [ uuid_produce shall copy the generated UUID's bytes in destination. ]*/
/*Tests_SRS_UUID_WIN32_02_004: [ uuid_produce shall succeed and return 0. ]*/
TEST_FUNCTION(uuid_produce_succeeds_1) /*when it returns default RPC_S_OK*/
{
    ///arrange
    int result;
    UUID_T u = { 0 };

    STRICT_EXPECTED_CALL(mocked_UuidCreate(IGNORED_ARG));

    ///act
    result = uuid_produce(u);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 24) & 0xFF, u[0]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 16) & 0xFF, u[1]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >>  8) & 0xFF, u[2]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1      ) & 0xFF, u[3]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2 >>  8) & 0xFF, u[4]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2      ) & 0xFF, u[5]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3 >>  8) & 0xFF, u[6]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3      ) & 0xFF, u[7]);

    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_0, u[8]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_1, u[9]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_2, u[10]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_3, u[11]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_4, u[12]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_5, u[13]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_6, u[14]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_7, u[15]);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_002: [ uuid_produce shall call UuidCreate to generate a UUID. ]*/
/*Tests_SRS_UUID_WIN32_02_003: [ uuid_produce shall copy the generated UUID's bytes in destination. ]*/
/*Tests_SRS_UUID_WIN32_02_004: [ uuid_produce shall succeed and return 0. ]*/
TEST_FUNCTION(uuid_produce_succeeds_2) /*when it returns default RPC_S_UUID_LOCAL_ONLY*/
{
    ///arrange
    int result;
    UUID_T u = { 0 };

    g_hook_UuidCreate_return = RPC_S_UUID_LOCAL_ONLY;
    STRICT_EXPECTED_CALL(mocked_UuidCreate(IGNORED_ARG));

    ///act
    result = uuid_produce(u);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 24) & 0xFF, u[0]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 16) & 0xFF, u[1]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 8) & 0xFF, u[2]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1) & 0xFF, u[3]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2 >> 8) & 0xFF, u[4]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2) & 0xFF, u[5]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3 >> 8) & 0xFF, u[6]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3) & 0xFF, u[7]);

    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_0, u[8]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_1, u[9]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_2, u[10]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_3, u[11]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_4, u[12]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_5, u[13]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_6, u[14]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_7, u[15]);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_005: [ If there are any failures then uuid_produce shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_produce_unhappy_path) /*when it returns RPC_S_UUID_LOCAL_ONLY*/
{
    ///arrange
    int result;
    UUID_T u = { 0 };

    g_hook_UuidCreate_return = RPC_S_UUID_NO_ADDRESS;
    STRICT_EXPECTED_CALL(mocked_UuidCreate(IGNORED_ARG));

    ///act
    result = uuid_produce(u);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_006: [ If destination is NULL then uuid_from_GUID shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_from_GUID_with_destination_NULL_fails)
{
    ///arrange
    int result;
    GUID source = { 0 };

    ///act
    result = uuid_from_GUID(NULL, &source);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_007: [ If source is NULL then uuid_from_GUID shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_from_GUID_with_source_NULL_fails)
{
    ///arrange
    int result;
    UUID_T destination;

    ///act
    result = uuid_from_GUID(destination, NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_008: [ uuid_from_GUID shall convert GUID to UUID_T, succeed and return 0. ]*/
TEST_FUNCTION(uuid_from_GUID_succeeds)
{
    ///arrange
    int result;
    GUID source = { .Data1 = TEST_DATA_1, .Data2 = TEST_DATA_2, .Data3 = TEST_DATA_3, .Data4 = {TEST_DATA_4_0, TEST_DATA_4_1, TEST_DATA_4_2, TEST_DATA_4_3, TEST_DATA_4_4, TEST_DATA_4_5, TEST_DATA_4_6, TEST_DATA_4_7} };
    UUID_T destination;

    ///act
    result = uuid_from_GUID(destination,&source);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 24) & 0xFF, destination[0]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 16) & 0xFF, destination[1]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1 >> 8) & 0xFF, destination[2]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_1) & 0xFF, destination[3]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2 >> 8) & 0xFF, destination[4]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_2) & 0xFF, destination[5]);

    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3 >> 8) & 0xFF, destination[6]);
    ASSERT_ARE_EQUAL(uint8_t, (TEST_DATA_3) & 0xFF, destination[7]);

    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_0, destination[8]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_1, destination[9]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_2, destination[10]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_3, destination[11]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_4, destination[12]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_5, destination[13]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_6, destination[14]);
    ASSERT_ARE_EQUAL(uint8_t, TEST_DATA_4_7, destination[15]);
}

/*Tests_SRS_UUID_WIN32_02_009: [ If destination is NULL then GUID_from_uuid shall fail and return a non-zero value. ]*/
TEST_FUNCTION(GUID_from_uuid_with_NULL_destination_fails)
{
    ///arrange
    int result;
    UUID_T source = { 0 };

    ///act
    result = GUID_from_uuid(NULL, source);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_010: [ If source is NULL then GUID_from_uuid shall fail and return a non-zero value. ]*/
TEST_FUNCTION(GUID_from_uuid_with_NULL_source_fails)
{
    ///arrange
    int result;
    GUID destination;

    ///act
    result = GUID_from_uuid(&destination, NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_WIN32_02_011: [ GUID_from_uuid shall convert UUID_T to GUID, succeed and return 0. ]*/
TEST_FUNCTION(GUID_from_uuid_succeeds)
{
    ///arrange
    int result;
    UUID_T source = { 
        ((TEST_DATA_1 >> 24) & 0xFF),
        ((TEST_DATA_1 >> 16) & 0xFF), 
        ((TEST_DATA_1 >>  8) & 0xFF), 
        ((TEST_DATA_1      ) & 0xFF), 
        ((TEST_DATA_2 >>  8) &0xFF),
        ((TEST_DATA_2      ) & 0xFF),
        ((TEST_DATA_3 >>  8) & 0xFF),
        ((TEST_DATA_3      ) & 0xFF),
        TEST_DATA_4_0, TEST_DATA_4_1, TEST_DATA_4_2, TEST_DATA_4_3, TEST_DATA_4_4, TEST_DATA_4_5, TEST_DATA_4_6, TEST_DATA_4_7};
    GUID destination;

    ///act
    result = GUID_from_uuid(&destination, source);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(TEST_DATA_1 == destination.Data1);
    ASSERT_IS_TRUE(TEST_DATA_2 == destination.Data2);
    ASSERT_IS_TRUE(TEST_DATA_3 == destination.Data3);
    ASSERT_IS_TRUE(0 == memcmp(source+8, destination.Data4, 8));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// is_uuid_nil

// Tests_SRS_UUID_WIN32_11_001: [ if uuid_value is NULL then is_uuid_nil shall fail and return true. ]
TEST_FUNCTION(is_uuid_nil_uuid_is_NULL)
{
    ///arrange

    ///act
    bool result = is_uuid_nil(NULL);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_UUID_WIN32_11_002: [ If all the values of is_uuid_nil are 0 then is_uuid_nil shall return true. ]
TEST_FUNCTION(is_uuid_nil_on_valid_uuid)
{
    ///arrange
    UUID_T valid_uuid = {
        ((TEST_DATA_1 >> 24) & 0xFF),
        ((TEST_DATA_1 >> 16) & 0xFF),
        ((TEST_DATA_1 >> 8) & 0xFF),
        ((TEST_DATA_1) & 0xFF),
        ((TEST_DATA_2 >> 8) & 0xFF),
        ((TEST_DATA_2) & 0xFF),
        ((TEST_DATA_3 >> 8) & 0xFF),
        ((TEST_DATA_3) & 0xFF),
        TEST_DATA_4_0, TEST_DATA_4_1, TEST_DATA_4_2, TEST_DATA_4_3, TEST_DATA_4_4, TEST_DATA_4_5, TEST_DATA_4_6, TEST_DATA_4_7 };

    ///act
    bool result = is_uuid_nil(valid_uuid);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_UUID_WIN32_11_003: [ If any the values of is_uuid_nil are not 0 then is_uuid_nil shall return false. ]
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

TEST_FUNCTION(is_uuid_nil_on_individual_valid_uuid)
{
    ///arrange
    for (size_t index = 0; index < 16; index++)
    {
        UUID_T valid_uuid = { 0 };

        valid_uuid[index] = 0x2;

        ///act
        bool result = is_uuid_nil(valid_uuid);

        ///assert
        ASSERT_IS_FALSE(result);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
