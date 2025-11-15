// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "interlocked_hl_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

MOCK_FUNCTION_WITH_CODE(, bool, TEST_IS_GREATER_32, int32_t, original_target, int32_t, exchange)
MOCK_FUNCTION_END(original_target < exchange)

MOCK_FUNCTION_WITH_CODE(, bool, TEST_IS_GREATER_64, int64_t, original_target, int64_t, exchange)
MOCK_FUNCTION_END(original_target<exchange)
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

typedef  struct ADDEND_AND_VALUE_TAG
{
    volatile_atomic int64_t Addend;
    int64_t Value;
} ADDEND_AND_VALUE;

typedef  struct ADDEND_CEILING_AND_VALUE_TAG
{
    volatile_atomic int64_t Addend;
    int64_t Ceiling;
    int64_t Value;
} ADDEND_CEILING_AND_VALUE;

typedef  struct INPUT_AND_OUTPUT_TAG
{
    volatile_atomic int32_t Input;
    int32_t Output;
} INPUT_AND_OUTPUT;

typedef  struct INPUT_AND_OUTPUT_64_TAG
{
    volatile_atomic int64_t Input;
    int64_t Output;
} INPUT_AND_OUTPUT_64;

static WAIT_ON_ADDRESS_RESULT hook_wait_on_address(volatile_atomic int32_t* address, int32_t compare_value, uint32_t milliseconds)
{
    (void)address;
    (void)compare_value;
    (void)milliseconds;
    return WAIT_ON_ADDRESS_OK;
}

static WAIT_ON_ADDRESS_RESULT hook_wait_on_address_64(volatile_atomic int64_t* address, int64_t compare_value, uint32_t milliseconds)
{
    (void)address;
    (void)compare_value;
    (void)milliseconds;
    return WAIT_ON_ADDRESS_OK;
}

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)


TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT);

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(wait_on_address, hook_wait_on_address);
    REGISTER_GLOBAL_MOCK_HOOK(wait_on_address_64, hook_wait_on_address_64);
}

TEST_SUITE_CLEANUP(b)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(d)
{
}

/* Tests_SRS_INTERLOCKED_HL_02_001: [ If Addend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ] */
/* Tests_SRS_INTERLOCKED_HL_02_007: [ In all failure cases InterlockedHL_Add64WithCeiling shall not modify Addend or originalAddend.]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_with_Addend_NULL_fails)
{
    ///arrange
    int64_t originalAddend=300;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(NULL, 10, 4, &originalAddend);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(int64_t, 300, originalAddend);

    ///cleanup
}

/* Tests_SRS_INTERLOCKED_HL_02_006: [ If originalAddend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_with_originalAddend_NULL_fails)
{
    ///arrange
    volatile_atomic int64_t Addend = 5;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&Addend, 10, 4, NULL);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(int64_t, 5, Addend);

    ///cleanup
}

/* Tests_SRS_INTERLOCKED_HL_02_002: [ If Addend + Value would underflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_underflow_it_fails)
{
    ///arrange
    int64_t originalAddend = 400;
    ADDEND_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Value*/
        { INT64_MIN,            -1},
        { INT64_MIN + 1,        -2},
        { -1,                   INT64_MIN },
        { INT64_MIN,            INT64_MIN}
    }, *cloneOfInputValues;

    cloneOfInputValues = real_gballoc_hl_malloc(sizeof(inputValues));
    ASSERT_IS_NOT_NULL(cloneOfInputValues);
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {

        ///arrange
        STRICT_EXPECTED_CALL(interlocked_add_64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), INT64_MAX, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 400, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    real_gballoc_hl_free(cloneOfInputValues);
}

/* Tests_SRS_INTERLOCKED_HL_02_003: [ If Addend + Value would overflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_overflow_it_fails)
{
    ///arrange
    int64_t originalAddend = 500;
    ADDEND_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Value*/
        { INT64_MAX,            1 },
        { INT64_MAX -1,         2 },
        { 1,                    INT64_MAX },
        { INT64_MAX,            INT64_MAX }
    }, *cloneOfInputValues;

    cloneOfInputValues = real_gballoc_hl_malloc(sizeof(inputValues));
    ASSERT_IS_NOT_NULL(cloneOfInputValues);
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {
        ///arrange
        STRICT_EXPECTED_CALL(interlocked_add_64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), INT64_MAX, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 500, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    real_gballoc_hl_free(cloneOfInputValues);
}

/* Tests_SRS_INTERLOCKED_HL_02_004: [ If Addend + Value would be greater than Ceiling then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_over_the_ceiling_it_fails)
{
    ///arrange
    int64_t originalAddend = 600;
    ADDEND_CEILING_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Ceiling*/             /*Value*/
        { INT64_MAX - 2,        INT64_MAX - 1,          2 },
        { 2,                    INT64_MAX - 1,          INT64_MAX - 2 },

        { -1,                   0,                      2 },
        { 2,                    0,                      -1 },

        { INT64_MIN,            INT64_MIN + 1,          2 },
        { 2,                    INT64_MIN + 1,          INT64_MIN },
    }, *cloneOfInputValues;

    cloneOfInputValues = real_gballoc_hl_malloc(sizeof(inputValues));
    ASSERT_IS_NOT_NULL(cloneOfInputValues);
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {
        ///arrange
        STRICT_EXPECTED_CALL(interlocked_add_64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), inputValues[i].Ceiling, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 600, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    real_gballoc_hl_free(cloneOfInputValues);
}

/* Tests_SRS_INTERLOCKED_HL_02_005: [ Otherwise, InterlockedHL_Add64WithCeiling shall atomically write in Addend the sum of Addend and Value, succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_succeeds)
{
    ///arrange
    int64_t originalAddend = 700;
    ADDEND_CEILING_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Ceiling*/             /*Value*/
        { INT64_MAX - 2,        INT64_MAX - 1,          1 },
        { 1,                    INT64_MAX - 1,          INT64_MAX - 2 },

        { -1,                   0,                      1 },
        { 1,                    0,                      -1 },

        { INT64_MIN,            INT64_MIN + 1,          1 },
        { 1,                    INT64_MIN + 1,          INT64_MIN },
    }, *cloneOfInputValues;

    cloneOfInputValues = real_gballoc_hl_malloc(sizeof(inputValues));
    ASSERT_IS_NOT_NULL(cloneOfInputValues);
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {

        ///arrange
        STRICT_EXPECTED_CALL(interlocked_add_64(&(inputValues[i].Addend), 0));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange_64(&(inputValues[i].Addend), IGNORED_ARG, inputValues[i].Addend));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), inputValues[i].Ceiling, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend+ cloneOfInputValues[i].Value, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    real_gballoc_hl_free(cloneOfInputValues);
}

/* InterlockedHL_WaitForValue */

/* Tests_SRS_INTERLOCKED_HL_01_002: [ If address_to_check is NULL, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_WaitForValue_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_WaitForValue(NULL, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_003: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_equals_target_value_InterlockedHL_WaitForValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_004: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue shall wait until the value at address_to_check changes using wait_on_address. ] */
/* Tests_SRS_INTERLOCKED_HL_01_007: [ When wait_on_address succeeds, InterlockedHL_WaitForValue shall again compare the value at address_to_check with value_to_wait. ] */
TEST_FUNCTION(when_the_value_equals_target_value_after_waiting_InterlockedHL_WaitForValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x41;
    int32_t target_value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&target_value, sizeof(int32_t));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_003: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ] */
/* Tests_SRS_INTERLOCKED_HL_01_007: [ When wait_on_address succeeds, InterlockedHL_WaitForValue shall again compare the value at address_to_check with value_to_wait. ] */
TEST_FUNCTION(when_the_value_after_a_succesfull_wait_on_address_does_not_equal_target_a_new_wait_for_address_shall_be_issued)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x40;
    int32_t intermediate_value = 0x41;
    int32_t final_value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int32_t));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&final_value, sizeof(int32_t));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_006: [ If wait_on_address fails, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(when_the_wait_on_address_fails_InterlockedHL_WaitForValue_also_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x40;
    int32_t intermediate_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int32_t))
        .SetReturn(WAIT_ON_ADDRESS_ERROR);

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_11_001: [ If wait_on_address hits the timeout specified in timeout_ms, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
TEST_FUNCTION(when_the_wait_on_address_timesout_InterlockedHL_WaitForValue_also_timesout)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x40;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, 1000))
        .SetReturn(WAIT_ON_ADDRESS_TIMEOUT);

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/* InterlockedHL_WaitForValue64 */

/* Tests_SRS_INTERLOCKED_HL_05_001: [ If address_to_check is NULL, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_WaitForValue64_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_WaitForValue64(NULL, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_002: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_equals_target_value_InterlockedHL_WaitForValue64_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_003: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_004: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForValue64 shall again compare the value at address_to_check with value_to_wait. */
/* Tests_SRS_INTERLOCKED_HL_05_002: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_equals_target_value_after_waiting_InterlockedHL_WaitForValue64_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x41;
    int64_t target_value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&target_value, sizeof(int64_t)); // Here we swap address of value with address of target_value
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_003: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_004: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForValue64 shall again compare the value at address_to_check with value_to_wait. */
/* Tests_SRS_INTERLOCKED_HL_05_002: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_after_a_succesfull_wait_on_address_64_does_not_equal_target_a_new_wait_for_address_64_shall_be_issued)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x40;
    int64_t intermediate_value = 0x41;
    int64_t final_value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int64_t));
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&final_value, sizeof(int64_t));
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_003: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_006: [ If wait_on_address_64 fails, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(when_the_wait_on_address_64_fails_InterlockedHL_WaitForValue64_also_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x40;
    int64_t intermediate_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int64_t))
        .SetReturn(WAIT_ON_ADDRESS_ERROR);

    // act
    result = InterlockedHL_WaitForValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_003: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_005: [ If wait_on_address_64 hits the timeout specified in timeout_ms, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
TEST_FUNCTION(when_the_wait_on_address_64_timesout_InterlockedHL_WaitForValue64_also_timesout)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x40;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, 1000))
        .SetReturn(WAIT_ON_ADDRESS_TIMEOUT);

    // act
    result = InterlockedHL_WaitForValue64(&value, 0x42, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}


/* InterlockedHL_WaitForNotValue */

/* Tests_SRS_INTERLOCKED_HL_42_001: [ If address_to_check is NULL, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_WaitForNotValue_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_WaitForNotValue(NULL, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_42_002: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_does_not_equal_target_value_InterlockedHL_WaitForNotValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x43;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_42_003: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue shall wait until the value at address_to_check changes by using wait_on_address. ] */
/* Tests_SRS_INTERLOCKED_HL_42_005: [ When wait_on_address succeeds, InterlockedHL_WaitForNotValue64 shall again compare the value at address_to_check with value_to_wait. ] */
TEST_FUNCTION(when_the_value_does_not_equal_target_value_after_waiting_InterlockedHL_WaitForNotValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x42;
    int32_t changed_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&changed_value, sizeof(changed_value));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_42_002: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue shall return INTERLOCKED_HL_OK. ] */
/* Tests_SRS_INTERLOCKED_HL_42_005: [ When wait_on_address succeeds, InterlockedHL_WaitForNotValue64 shall again compare the value at address_to_check with value_to_wait. ] */
TEST_FUNCTION(when_the_value_after_a_succesfull_wait_on_address_equals_target_a_new_wait_for_address_shall_be_issued)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x42;
    int32_t intermediate_value = 0x42;
    int32_t final_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int32_t));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&final_value, sizeof(int32_t));
    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_42_007: [ If wait_on_address fails, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(when_the_wait_on_address_fails_InterlockedHL_WaitForNotValue_also_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .SetReturn(WAIT_ON_ADDRESS_ERROR);

    // act
    result = InterlockedHL_WaitForNotValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_11_002: [ If wait_on_address hits the timeout specified in timeout_ms, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
TEST_FUNCTION(when_the_wait_on_address_timesout_InterlockedHL_WaitForNotValue_also_timesout)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int32_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address(&value, IGNORED_ARG, UINT32_MAX))
        .SetReturn(WAIT_ON_ADDRESS_TIMEOUT);

    // act
    result = InterlockedHL_WaitForNotValue(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/* InterlockedHL_WaitForNotValue64 */

/* Tests_SRS_INTERLOCKED_HL_05_007: [ If address_to_check is NULL, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_WaitForNotValue64_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_WaitForNotValue64(NULL, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_008: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_does_not_equal_target_value_InterlockedHL_WaitForNotValue64_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x43;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_009: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_010: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForNotValue64 shall again compare the value at address_to_check with value_to_wait. ] */
/* Tests_SRS_INTERLOCKED_HL_05_008: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_does_not_equal_target_value_after_waiting_InterlockedHL_WaitForNotValue64_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x42;
    int64_t changed_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&changed_value, sizeof(changed_value));
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_009: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_010: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForNotValue64 shall again compare the value at address_to_check with value_to_wait. ] */
/* Tests_SRS_INTERLOCKED_HL_05_008: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(when_the_value_after_a_succesfull_wait_on_address_64_equals_target_a_new_wait_for_address_64_shall_be_issued)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x42;
    int64_t intermediate_value = 0x42;
    int64_t final_value = 0x41;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&intermediate_value, sizeof(int64_t));
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .CopyOutArgumentBuffer_address(&final_value, sizeof(int64_t));
    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));

    // act
    result = InterlockedHL_WaitForNotValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_009: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_012: [ If wait_on_address_64 fails, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(when_the_wait_on_address_64_fails_InterlockedHL_WaitForNotValue64_also_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .SetReturn(WAIT_ON_ADDRESS_ERROR);

    // act
    result = InterlockedHL_WaitForNotValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_009: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
/* Tests_SRS_INTERLOCKED_HL_05_011: [ If wait_on_address_64 hits the timeout specified in timeout_ms, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
TEST_FUNCTION(when_the_wait_on_address_64_timesout_InterlockedHL_WaitForNotValue64_also_timesout)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    volatile_atomic int64_t value = 0x42;

    STRICT_EXPECTED_CALL(interlocked_add_64(&value, 0));
    STRICT_EXPECTED_CALL(wait_on_address_64(&value, IGNORED_ARG, UINT32_MAX))
        .SetReturn(WAIT_ON_ADDRESS_TIMEOUT);

    // act
    result = InterlockedHL_WaitForNotValue64(&value, 0x42, UINT32_MAX);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/* InterlockedHL_CompareExchangeIf */

/* Tests_SRS_INTERLOCKED_HL_01_009: [ If target is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_target_NULL_fails)
{
    ///arrange
    int32_t original_target;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(NULL, 1, TEST_IS_GREATER_32, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_01_010: [ If compare is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_compare_NULL_fails)
{
    ///arrange
    int32_t original_target;
    volatile_atomic int32_t target;
    (void)interlocked_exchange(&target, 34);
    umock_c_reset_all_calls();

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(&target, 1, NULL, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_01_011: [ If original_target is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_original_target_NULL_fails)
{
    ///arrange
    volatile_atomic int32_t target;
    (void)interlocked_exchange(&target, 34);
    umock_c_reset_all_calls();

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(&target, 1, TEST_IS_GREATER_32, NULL);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_01_012: [ InterlockedHL_CompareExchangeIf shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_01_013: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchangeIf shall exchange target with exchange. ] */
/* Tests_SRS_INTERLOCKED_HL_01_015: [ If target did not change meanwhile then InterlockedHL_CompareExchangeIf shall return INTERLOCKED_HL_OK and shall peform the exchange of values. ] */
/* Tests_SRS_INTERLOCKED_HL_01_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_compare_true_changed_false_succeeds)
{
    ///arrange
    int32_t original_target;
    volatile_atomic int32_t target;
    (void)interlocked_exchange(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_32(34, 99));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&target, 99, 34));

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(&target, 99, TEST_IS_GREATER_32, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int32_t, 34, original_target);
    ASSERT_ARE_EQUAL(int32_t, 99, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_01_012: [ InterlockedHL_CompareExchangeIf shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_01_013: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchangeIf shall exchange target with exchange. ] */
/* Tests_SRS_INTERLOCKED_HL_01_014: [ If target changed meanwhile then InterlockedHL_CompareExchangeIf shall return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ] */
/* Tests_SRS_INTERLOCKED_HL_01_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_compare_true_changed_true_succeeds)
{
    ///arrange
    int32_t original_target;
    volatile_atomic int32_t target;
    (void)interlocked_exchange(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_32(34, 99));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&target, 99, 34))
        .SetReturn(35);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(&target, 99, TEST_IS_GREATER_32, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_CHANGED, result);
    ASSERT_ARE_EQUAL(int32_t, 34, original_target);
    ASSERT_ARE_EQUAL(int32_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_01_012: [ InterlockedHL_CompareExchangeIf shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_01_017: [ If compare returns false then  InterlockedHL_CompareExchangeIf shall not perform any exchanges and return INTERLOCKED_HL_OK. ] */
/* Tests_SRS_INTERLOCKED_HL_01_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchangeIf_with_compare_false_succeeds)
{
    ///arrange
    int32_t original_target;
    volatile_atomic int32_t target;
    (void)interlocked_exchange(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_32(34, 33)); /*33 is smaller, no exchanges happen*/

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchangeIf(&target, 33, TEST_IS_GREATER_32, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int32_t, 34, original_target);
    ASSERT_ARE_EQUAL(int32_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* InterlockedHL_CompareExchange64If */

/* Tests_SRS_INTERLOCKED_HL_02_008: [ If target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_target_NULL_fails)
{
    ///arrange
    int64_t original_target;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(NULL, 1, TEST_IS_GREATER_64, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_02_009: [ If compare is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_compare_NULL_fails)
{
    ///arrange
    int64_t original_target;
    volatile_atomic int64_t target;
    (void)interlocked_exchange_64(&target, 34);
    umock_c_reset_all_calls();

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 1, NULL, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_02_010: [ If original_target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_original_target_NULL_fails)
{
    ///arrange
    volatile_atomic int64_t target;
    (void)interlocked_exchange_64(&target, 34);
    umock_c_reset_all_calls();

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 1, TEST_IS_GREATER_64, NULL);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ] */
/* Tests_SRS_INTERLOCKED_HL_02_014: [ If target did not change meanwhile then InterlockedHL_CompareExchange64If shall return INTERLOCKED_HL_OK and shall peform the exchange of values. ] */
/* Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_compare_true_changed_false_succeeds)
{
    ///arrange
    int64_t original_target;
    volatile_atomic int64_t target;
    (void)interlocked_exchange_64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add_64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_64(34, 99));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange_64(&target, 99, 34));

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 99, TEST_IS_GREATER_64, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 99, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ] */
/* Tests_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ] */
/* Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_compare_true_changed_true_succeeds)
{
    ///arrange
    int64_t original_target;
    volatile_atomic int64_t target;
    (void)interlocked_exchange_64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add_64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_64(34, 99));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange_64(&target, 99, 34))
        .SetReturn(35);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 99, TEST_IS_GREATER_64, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_CHANGED, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ] */
/* Tests_SRS_INTERLOCKED_HL_02_015: [ If compare returns false then  InterlockedHL_CompareExchange64If shall not perform any exchanges and return INTERLOCKED_HL_OK. ] */
/* Tests_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ] */
/* Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ] */
TEST_FUNCTION(InterlockedHL_CompareExchange64If_with_compare_false_succeeds)
{
    ///arrange
    int64_t original_target;
    volatile_atomic int64_t target;
    (void)interlocked_exchange_64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add_64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER_64(34, 33)); /*33 is smaller, no exchanges happen*/

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 33, TEST_IS_GREATER_64, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


/* InterlockedHL_SetAndWake */

/* Tests_SRS_INTERLOCKED_HL_02_020: [ If address is NULL then InterlockedHL_SetAndWake shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_SetAndWake_with_address_NULL_fails)
{
    ///arrange
    INTERLOCKED_HL_RESULT result;

    ///act
    result = InterlockedHL_SetAndWake(NULL, 3);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_02_017: [ InterlockedHL_SetAndWake shall set address to value. ] */
/* Tests_SRS_INTERLOCKED_HL_02_018: [ InterlockedHL_SetAndWake shall call wake_by_address_single. ] */
/* Tests_SRS_INTERLOCKED_HL_02_019: [ InterlockedHL_SetAndWake shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_SetAndWake_succeeds)
{
    ///arrange
    volatile_atomic int32_t hereLiesAThree = 3;
    int32_t thatIsGoingToTurn4 = 4;
    INTERLOCKED_HL_RESULT result;

    STRICT_EXPECTED_CALL(interlocked_exchange(&hereLiesAThree, thatIsGoingToTurn4));
    STRICT_EXPECTED_CALL(wake_by_address_single(&hereLiesAThree));

    ///act
    result = InterlockedHL_SetAndWake(&hereLiesAThree, thatIsGoingToTurn4);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* InterlockedHL_SetAndWake64 */

/* Tests_SRS_INTERLOCKED_HL_05_013: [ If address is NULL then InterlockedHL_SetAndWake64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_SetAndWake64_with_address_NULL_fails)
{
    ///arrange
    INTERLOCKED_HL_RESULT result;

    ///act
    result = InterlockedHL_SetAndWake64(NULL, 3);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_014: [ InterlockedHL_SetAndWake64 shall set value at address. ] */
/* Tests_SRS_INTERLOCKED_HL_05_015: [ InterlockedHL_SetAndWake64 shall wake a single thread listening on address. ] */
/* Tests_SRS_INTERLOCKED_HL_05_016: [ InterlockedHL_SetAndWake64 shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_SetAndWake64_succeeds)
{
    ///arrange
    volatile_atomic int64_t hereLiesAThree = 3;
    int64_t thatIsGoingToTurn4 = 4;
    INTERLOCKED_HL_RESULT result;

    STRICT_EXPECTED_CALL(interlocked_exchange_64(&hereLiesAThree, thatIsGoingToTurn4));
    STRICT_EXPECTED_CALL(wake_by_address_single_64(&hereLiesAThree));

    ///act
    result = InterlockedHL_SetAndWake64(&hereLiesAThree, thatIsGoingToTurn4);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* InterlockedHL_SetAndWakeAll */

/* Tests_SRS_INTERLOCKED_HL_02_028: [ If address is NULL then InterlockedHL_SetAndWakeAll shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_SetAndWakeAll_with_address_NULL_fails)
{
    ///arrange
    INTERLOCKED_HL_RESULT result;

    ///act
    result = InterlockedHL_SetAndWakeAll(NULL, 3);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_02_029: [ InterlockedHL_SetAndWakeAll shall set address to value. ] */
/* Tests_SRS_INTERLOCKED_HL_02_030: [ InterlockedHL_SetAndWakeAll shall call wake_by_address_all. ] */
/* Tests_SRS_INTERLOCKED_HL_02_031: [ InterlockedHL_SetAndWakeAll shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_SetAndWakeAll_succeeds)
{
    ///arrange
    volatile_atomic int32_t hereLiesAThree = 3;
    int32_t thatIsGoingToTurn4 = 4;
    INTERLOCKED_HL_RESULT result;

    STRICT_EXPECTED_CALL(interlocked_exchange(&hereLiesAThree, thatIsGoingToTurn4));
    STRICT_EXPECTED_CALL(wake_by_address_all(&hereLiesAThree));

    ///act
    result = InterlockedHL_SetAndWakeAll(&hereLiesAThree, thatIsGoingToTurn4);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* InterlockedHL_SetAndWakeAll64 */

/* Tests_SRS_INTERLOCKED_HL_05_017: [ If address is NULL then InterlockedHL_SetAndWakeAll64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_SetAndWakeAll64_with_address_NULL_fails)
{
    ///arrange
    INTERLOCKED_HL_RESULT result;

    ///act
    result = InterlockedHL_SetAndWakeAll64(NULL, 3);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_018: [ InterlockedHL_SetAndWakeAll64 shall set value at address. ] */
/* Tests_SRS_INTERLOCKED_HL_05_019: [ InterlockedHL_SetAndWakeAll64 shall wake all threads listening on address. ] */
/* Tests_SRS_INTERLOCKED_HL_05_020: [ InterlockedHL_SetAndWakeAll64 shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_SetAndWakeAll64_succeeds)
{
    ///arrange
    volatile_atomic int64_t hereLiesAThree = 3;
    int64_t thatIsGoingToTurn4 = 4;
    INTERLOCKED_HL_RESULT result;

    STRICT_EXPECTED_CALL(interlocked_exchange_64(&hereLiesAThree, thatIsGoingToTurn4));
    STRICT_EXPECTED_CALL(wake_by_address_all_64(&hereLiesAThree));

    ///act
    result = InterlockedHL_SetAndWakeAll64(&hereLiesAThree, thatIsGoingToTurn4);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* InterlockedHL_DecrementAndWake */

/* Tests_SRS_INTERLOCKED_HL_44_001: [ If address is NULL then InterlockedHL_DecrementAndWake shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_DecrementAndWake_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_DecrementAndWake(NULL);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_44_002: [ InterlockedHL_DecrementAndWake shall decrement the value at address by 1. ] */
/* Tests_SRS_INTERLOCKED_HL_44_003: [ InterlockedHL_DecrementAndWake shall call wake_by_address_single. ] */
/* Tests_SRS_INTERLOCKED_HL_44_004: [ InterlockedHL_DecrementAndWake shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_DecrementAndWake_with_Valid_Inputs_success)
{
    ///arrange
    INPUT_AND_OUTPUT testCases[] =
    {
        /*Input*/              /*Output*/
        { INT32_MAX,            INT32_MAX - 1},
        { INT32_MAX - 1,        INT32_MAX - 2},
        { INT32_MIN,            INT32_MAX},
        { INT32_MIN + 1,        INT32_MIN},
        { -1,                   -2 },
        { 0,                    -1} ,
        { 1,                    0},
        { 1024,                 1023 },
        { -1024,                -1025 },
    };
    size_t nTestCases = sizeof(testCases) / sizeof(testCases[0]);
    for (size_t i = 0; i < nTestCases; i++)
    {
        ///arrange
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(interlocked_decrement(&(testCases[i].Input)));
        STRICT_EXPECTED_CALL(wake_by_address_single(&(testCases[i].Input)));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_DecrementAndWake(&(testCases[i].Input));

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
        ASSERT_ARE_EQUAL(int64_t, testCases[i].Output, testCases[i].Input);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
}

/* InterlockedHL_DecrementAndWake64 */

/* Tests_SRS_INTERLOCKED_HL_05_021: [ If address is NULL then InterlockedHL_DecrementAndWake64 shall fail and return INTERLOCKED_HL_ERROR. ] */
TEST_FUNCTION(InterlockedHL_DecrementAndWake64_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_DecrementAndWake64(NULL);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_05_022: [ InterlockedHL_DecrementAndWake64 shall decrement the value at address by 1. ] */
/* Tests_SRS_INTERLOCKED_HL_05_023: [ InterlockedHL_DecrementAndWake64 shall wake a single thread listening on address. ] */
/* Tests_SRS_INTERLOCKED_HL_05_024: [ InterlockedHL_DecrementAndWake64 shall succeed and return INTERLOCKED_HL_OK. ] */
TEST_FUNCTION(InterlockedHL_DecrementAndWake64_with_Valid_Inputs_success)
{
    ///arrange
    INPUT_AND_OUTPUT_64 testCases[] =
    {
        /*Input*/              /*Output*/
        { INT64_MAX,            INT64_MAX - 1},
        { INT64_MAX - 1,        INT64_MAX - 2},
        { INT64_MIN,            INT64_MAX},
        { INT64_MIN + 1,        INT64_MIN},
        { -1,                   -2 },
        { 0,                    -1} ,
        { 1,                    0},
        { 1024,                 1023 },
        { -1024,                -1025 },
    };
    size_t nTestCases = sizeof(testCases) / sizeof(testCases[0]);
    for (size_t i = 0; i < nTestCases; i++)
    {
        ///arrange
        umock_c_reset_all_calls();
        STRICT_EXPECTED_CALL(interlocked_decrement_64(&(testCases[i].Input)));
        STRICT_EXPECTED_CALL(wake_by_address_single_64(&(testCases[i].Input)));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_DecrementAndWake64(&(testCases[i].Input));

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
        ASSERT_ARE_EQUAL(int64_t, testCases[i].Output, testCases[i].Input);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
