// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <uuid.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, uuid_rc_t, mocked_uuid_create, uuid_t**, out);
MOCKABLE_FUNCTION(, uuid_rc_t, mocked_uuid_make, uuid_t*, out, unsigned int, mode);
MOCKABLE_FUNCTION(, uuid_rc_t, mocked_uuid_export, const uuid_t *,uuid, uuid_fmt_t, fmt, void *,data_ptr, size_t *,data_len);
MOCKABLE_FUNCTION(, uuid_rc_t, mocked_uuid_destroy, uuid_t*, out);
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


static uuid_rc_t hook_uuid_create(uuid_t** out)
{
    unsigned char * uuid_ptr = malloc(16);
    ASSERT_IS_NOT_NULL(uuid_ptr);
    uuid_ptr[0]  = TEST_DATA_0 ;
    uuid_ptr[1]  = TEST_DATA_1 ;
    uuid_ptr[2]  = TEST_DATA_2 ;
    uuid_ptr[3]  = TEST_DATA_3 ;
    uuid_ptr[4]  = TEST_DATA_4 ;
    uuid_ptr[5]  = TEST_DATA_5 ;
    uuid_ptr[6]  = TEST_DATA_6 ;
    uuid_ptr[7]  = TEST_DATA_7 ;
    uuid_ptr[8]  = TEST_DATA_8 ;
    uuid_ptr[9]  = TEST_DATA_9 ;
    uuid_ptr[10] = TEST_DATA_10;
    uuid_ptr[11] = TEST_DATA_11;
    uuid_ptr[12] = TEST_DATA_12;
    uuid_ptr[13] = TEST_DATA_13;
    uuid_ptr[14] = TEST_DATA_14;
    uuid_ptr[15] = TEST_DATA_15;
    *out = (uuid_t*)uuid_ptr;
    return UUID_RC_OK;
}

static uuid_rc_t hook_uuid_make(uuid_t* out, unsigned int mode)
{
    return UUID_RC_OK;
}

static uuid_rc_t hook_uuid_export(const uuid_t *uuid, uuid_fmt_t fmt, void *data_ptr, size_t *data_len)
{
    unsigned char * out = *(unsigned char **)data_ptr;
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
    return UUID_RC_OK;
}

static uuid_rc_t hook_uuid_export_short_copy(const uuid_t *uuid, uuid_fmt_t fmt, void *data_ptr, size_t *data_len)
{
    (void)hook_uuid_export(uuid, fmt, data_ptr, data_len);
    *data_len=sizeof(UUID_T) -1;
    return UUID_RC_OK;
}

static uuid_rc_t hook_uuid_destroy( uuid_t* out)
{
    free(out);
}

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static char* umocktypes_stringify_uuid_t(const uuid_t* value)
{
    char* result;
    UUID_T* uuid_t_value = (UUID_T*)value;
    result = malloc(37);
    ASSERT_IS_NOT_NULL(result);
    (void)sprintf(result, "%" PRI_UUID_T "", UUID_T_VALUES(*uuid_t_value));
    return result;
}

static int umocktypes_are_equal_uuid_t(const uuid_t* left, const uuid_t* right)
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
            result = (memcmp(*(unsigned char **)left, *(unsigned char **)right, sizeof(UUID_T)) == 0);
        }
    }
    return result;
}

static int umocktypes_copy_uuid_t(uuid_t* destination, const uuid_t* source)
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
        (void)memcpy(*(unsigned char **)destination, (unsigned char *)source, sizeof(UUID_T));
        return 0;
    }
}

static void umocktypes_free_uuid_t(uuid_t* value)
{
    free(value);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_TYPE(uuid_t, uuid_t);
    REGISTER_UMOCK_ALIAS_TYPE(uuid_rc_t, int);
    REGISTER_UMOCK_ALIAS_TYPE(uuid_fmt_t, int);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_create, hook_uuid_create);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_make, hook_uuid_make);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, hook_uuid_export);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_destroy, hook_uuid_destroy);

}

TEST_SUITE_CLEANUP(suite_cleanup)
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
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_UUID_02_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
/*Tests_SRS_UUID_LINUX_45_001: [ If destination is NULL then uuid_produce shall fail and return a non-NULL value. ]*/
TEST_FUNCTION(uuid_produce_with_destination_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = uuid_produce(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

}

/*Tests_SRS_UUID_02_002: [ uuid_produce shall generate in destination the representation of a UUID (as per RFC 4122). ]*/
/*Tests_SRS_UUID_02_004: [ uuid_produce shall succeed and return 0. ]*/
/*Tests_SRS_UUID_LINUX_45_003: [ uuid_produce shall call uuid_create to allocate a uuid. ]*/
/*Tests_SRS_UUID_LINUX_45_004: [ uuid_produce shall call uuid_make with mode = UUID_MAKE_V4 to generate the UUID value. ]*/
/*Tests_SRS_UUID_LINUX_45_005: [ uuid_produce shall call uuid_export with fmt = UUID_FMT_BIN to load the value into the destination. ]*/
/*Tests_SRS_UUID_LINUX_45_006: [ uuid_produce shall call uuid_destroy to deallocate the uuid. ]*/
/*Tests_SRS_UUID_LINUX_45_002: [ If all uuid calls return success, uuid_produce shall succeed and return 0. ]*/
TEST_FUNCTION(uuid_produce_succeeds)
{
    ///arrange
    int result;
    UUID_T u = { 0 };

    STRICT_EXPECTED_CALL(mocked_uuid_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_make(IGNORED_ARG, UUID_MAKE_V4));
    STRICT_EXPECTED_CALL(mocked_uuid_export(IGNORED_ARG, UUID_FMT_BIN, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_destroy(IGNORED_ARG));

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

/*Tests_SRS_UUID_LINUX_45_008: [ If uuid_export does not write size_of(UUID_T) bytes into the destination, the call shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_produce_returns_failure_when_copy_fall_short)
{
    ///arrange
    int result;
    UUID_T u = { 0 };
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, hook_uuid_export_short_copy);

    STRICT_EXPECTED_CALL(mocked_uuid_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_make(IGNORED_ARG, UUID_MAKE_V4));
    STRICT_EXPECTED_CALL(mocked_uuid_export(IGNORED_ARG, UUID_FMT_BIN, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_destroy(IGNORED_ARG));

    ///act
    result = uuid_produce(u);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // Cleanup
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, hook_uuid_export);
}

/*Tests_SRS_UUID_LINUX_45_007: [ If any uuid call returns a value other than UUID_RC_OK, the call shall fail and return a non-zero value. ]*/
TEST_FUNCTION(uuid_negative_tests)
{
        ///arrange
    int result;
    UUID_T u = { 0 };
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_make, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_destroy, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(mocked_uuid_create, UUID_RC_OK);
    REGISTER_GLOBAL_MOCK_RETURN(mocked_uuid_make, UUID_RC_OK);
    REGISTER_GLOBAL_MOCK_RETURN(mocked_uuid_export, UUID_RC_OK);
    REGISTER_GLOBAL_MOCK_RETURN(mocked_uuid_destroy, UUID_RC_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_uuid_create, UUID_RC_MEM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_uuid_make, UUID_RC_MEM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_uuid_export, UUID_RC_MEM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_uuid_destroy, UUID_RC_MEM);

    STRICT_EXPECTED_CALL(mocked_uuid_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_make(IGNORED_ARG, UUID_MAKE_V4));
    STRICT_EXPECTED_CALL(mocked_uuid_export(IGNORED_ARG, UUID_FMT_BIN, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_uuid_destroy(IGNORED_ARG));

    umock_c_negative_tests_snapshot();
    size_t i = 0;
    // (-1 because call to uuid_destroy is not checked)
    for (i = 0; i < umock_c_negative_tests_call_count() - 1; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = uuid_produce(u);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed uuid_produce call %zu", i);
        }
    }

    // cleanup
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_create, hook_uuid_create);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_make, hook_uuid_make);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_export, hook_uuid_export);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_uuid_destroy, hook_uuid_destroy);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
