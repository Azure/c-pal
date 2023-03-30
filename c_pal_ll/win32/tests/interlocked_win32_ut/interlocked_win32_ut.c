// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

#include "c_pal/interlocked.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define ENABLE_MOCKS
#include "mock_interlocked.h"
#undef ENABLE_MOCKS

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());
    REGISTER_UMOCK_ALIAS_TYPE(SHORT, int16_t);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/*Tests_SRS_INTERLOCKED_WIN32_43_001: [interlocked_add shall call InterlockedAdd from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_002 : [interlocked_add shall return the result of the addition.]*/
TEST_FUNCTION(interlocked_add_calls_InterlockedAdd)
{
    ///arrange
    volatile int32_t addend;
    InterlockedExchange((volatile LONG*)&addend, INT32_MAX);
    int32_t value = INT32_MIN;
    STRICT_EXPECTED_CALL(mock_InterlockedAdd((volatile LONG*)&addend, value))
        .SetReturn(-1);

    ///act
    int32_t return_val = interlocked_add(&addend, value);


    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_064: [ interlocked_add_64 shall call InterlockedAdd64 from windows.h. ]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_065: [ interlocked_add_64 shall return the result of the addition. ]*/
TEST_FUNCTION(interlocked_add_64_calls_InterlockedAdd64)
{
    ///arrange
    volatile int64_t addend;
    InterlockedExchange64(&addend, INT64_MAX);
    int64_t value = INT64_MIN;
    STRICT_EXPECTED_CALL(mock_InterlockedAdd64((volatile LONG64*)&addend, value))
        .SetReturn(-1);

    ///act
    int64_t return_val = interlocked_add_64(&addend, value);


    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_003: [interlocked_and shall call InterlockedAnd from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_004 : [interlocked_and shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_and_calls_InterlockedAnd)
{
    ///arrange
    volatile uint32_t destination;
    InterlockedExchange((volatile LONG*)&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedAnd((volatile LONG*)&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_and((volatile int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_005: [interlocked_and_16 shall call InterlockedAnd16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_006 : [interlocked_and_16 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_and_16_calls_InterlockedAnd16)
{
    ///arrange
    volatile uint16_t destination;
    InterlockedExchange16((volatile int16_t*)&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedAnd16((volatile SHORT*)&destination, value))
        .SetReturn((uint16_t)0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_and_16((volatile int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_007: [interlocked_and_64 shall call InterlockedAnd64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_008 : [interlocked_and_64 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_and_64_calls_InterlockedAnd64)
{
    ///arrange
    volatile uint64_t destination;
    InterlockedExchange64((volatile LONG64*)&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedAnd64((volatile LONG64*)&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_and_64((volatile int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_009: [interlocked_and_8 shall call InterlockedAnd8 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_010 : [interlocked_and_8 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_and_8_calls_InterlockedAdd8)
{
    ///arrange
    volatile uint8_t destination;
    InterlockedExchange8((volatile CHAR*)&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_InterlockedAnd8((volatile char*)&destination, value))
        .SetReturn((uint8_t)0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_and_8((volatile int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_011: [interlocked_compare_exchange shall call InterlockedCompareExchange from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_012 : [interlocked_compare_exchange shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_calls_InterlockedCompareExchange)
{
    ///arrange
    volatile int32_t destination;
    InterlockedExchange((volatile LONG*)&destination, INT32_MAX);
    int32_t comperand = INT32_MAX;
    int32_t exchange = INT32_MIN;
    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchange((volatile LONG*)&destination, exchange, comperand))
        .SetReturn(INT32_MAX);

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect.");
}

#ifdef _WIN64
/*Tests_SRS_INTERLOCKED_WIN32_43_013: [interlocked_compare_exchange_128 shall call InterlockedCompareExchange128 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_014 : [interlocked_compare_exchange_128 shall return true if* comperand_result equals the original value of* destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_128_calls_InterlockedCompareExchange128_true_case)
{
    ///arrange
    volatile int64_t* destination;
    InterlockedExchangePointer((PVOID volatile*)&destination, malloc(2 * sizeof(int64_t)));
    ASSERT_IS_NOT_NULL(destination);
    int64_t* comperand_result = malloc(2 * sizeof(int64_t));
    ASSERT_IS_NOT_NULL(comperand_result);

    destination[0] = INT64_MAX;
    destination[1] = INT64_MIN;
    comperand_result[0] = INT64_MAX;
    comperand_result[1] = INT64_MIN;

    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchange128((volatile LONG64*)destination, exchange_high, exchange_low, comperand_result))
        .SetReturn(TRUE);

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_IS_TRUE(return_val, "Return value is incorrect.");

    ///cleanup
    free((void*)destination);
    free(comperand_result);
}

/*Tests_SRS_INTERLOCKED_WIN32_43_013: [interlocked_compare_exchange_128 shall call InterlockedCompareExchange128 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_063 : [interlocked_compare_exchange_128 shall return false if* comperand_result does not equal the original value of* destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_128_calls_InterlockedCompareExchange128_false_case)
{
    ///arrange
    volatile int64_t* destination;
    InterlockedExchangePointer((PVOID volatile*)&destination, malloc(2 * sizeof(int64_t)));
    int64_t* comperand_result = malloc(2 * sizeof(int64_t));
    ASSERT_IS_NOT_NULL(comperand_result);

    destination[0] = INT64_MAX;
    destination[1] = INT64_MIN;
    comperand_result[0] = INT64_MAX;
    comperand_result[1] = INT64_MIN;

    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchange128((volatile LONG64*)destination, exchange_high, exchange_low, comperand_result))
        .SetReturn(FALSE);

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_IS_FALSE(return_val, "Return value is incorrect.");

    ///cleanup
    free((void*)destination);
    free(comperand_result);
}
#endif

/*Tests_SRS_INTERLOCKED_WIN32_43_015: [interlocked_compare_exchange_16 shall call InterlockedCompareExchange16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_016 : [interlocked_compare_exchange_16 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_16_calls_InterlockedCompareExchange16)
{
    ///arrange
    volatile int16_t destination;
    InterlockedExchange16(&destination, INT16_MAX);
    int16_t comperand = INT16_MAX;
    int16_t exchange = INT16_MIN;
    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchange16((volatile SHORT*)&destination, exchange, comperand))
        .SetReturn(INT16_MAX);

    ///act
    int16_t return_val = interlocked_compare_exchange_16(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_017: [interlocked_compare_exchange_64 shall call InterlockedCompareExchange64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_018 : [interlocked_compare_exchange_64 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_64_calls_InterlockedCompareExchange64)
{
    ///arrange
    volatile int64_t destination;
    InterlockedExchange64(&destination, INT64_MAX);
    int64_t comperand = INT64_MAX;
    int64_t exchange = INT64_MIN;
    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchange64((volatile LONG64*)&destination, exchange, comperand))
        .SetReturn(INT64_MAX);

    ///act
    int64_t return_val = interlocked_compare_exchange_64(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_019: [interlocked_compare_exchange_pointer shall call InterlockedCompareExchangePointer from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_020 : [interlocked_compare_exchange_pointer shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_pointer_calls_InterlockedCompareExchangePointer)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile destination;
    InterlockedExchangePointer((PVOID volatile*)&destination, &value1);
    void* comperand = &value1;
    void* exchange = &value2;
    STRICT_EXPECTED_CALL(mock_InterlockedCompareExchangePointer((PVOID volatile*)&destination, exchange, comperand))
        .SetReturn(&value1);

    ///act
    void* return_val = interlocked_compare_exchange_pointer(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_021: [interlocked_decrement shall call InterlockedDecrement from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_022 : [interlocked_decrement shall return the resulting 32 - bit integer value.]*/
TEST_FUNCTION(interlocked_decrement_calls_InterlockedDecrement)
{
    ///arrange
    volatile int32_t addend;
    InterlockedExchange((volatile LONG*)&addend, INT32_MAX);
    STRICT_EXPECTED_CALL(mock_InterlockedDecrement((volatile LONG*)&addend))
        .SetReturn(INT32_MAX -1);

    ///act
    int32_t return_val = interlocked_decrement(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_023: [interlocked_decrement_16 shall call InterlockedDecrement16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_024 : [interlocked_decrement_16 shall return the resulting 16 - bit integer value.]*/
TEST_FUNCTION(interlocked_decrement_16_calls_InterlockedDecrement16)
{
    ///arrange
    volatile int16_t addend;
    InterlockedExchange16(&addend, INT16_MAX);
    STRICT_EXPECTED_CALL(mock_InterlockedDecrement16((volatile SHORT*)&addend))
        .SetReturn(INT16_MAX -1);

    ///act
    int16_t return_val = interlocked_decrement_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_025: [interlocked_decrement_64 shall call InterlockedDecrement64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_026 : [interlocked_decrement_64 shall return the resulting 64 - bit integer value.]*/
TEST_FUNCTION(interlocked_decrement_64_calls_InterlockedDecrement64)
{
    ///arrange
    volatile int64_t addend;
    InterlockedExchange64(&addend, INT64_MAX);
    STRICT_EXPECTED_CALL(mock_InterlockedDecrement64((volatile LONG64*)&addend))
        .SetReturn(INT64_MAX - 1);

    ///act
    int64_t return_val = interlocked_decrement_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX - 1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_027: [interlocked_exchange shall call InterlockedExchange from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_028 : [interlocked_exchange shall return the initial value pointed to by target.]*/
TEST_FUNCTION(interlocked_exchange_calls_InterlockedExchange)
{
    ///arrange
    volatile int32_t target;
    InterlockedExchange((volatile LONG*)&target, INT32_MIN);
    int32_t value = INT32_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchange((volatile LONG*)&target, value))
        .SetReturn(INT32_MIN);

    ///act
    int32_t return_val = interlocked_exchange(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_029: [interlocked_exchange_16 shall call InterlockedExchange16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_030 : [interlocked_exchange_16 shall return the initial value pointed to by target.]*/
TEST_FUNCTION(interlocked_exchange_16_calls_InterlockedExchange16)
{
    ///arrange
    volatile int16_t target;
    InterlockedExchange16(&target, INT16_MIN);
    int16_t value = INT16_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchange16((volatile SHORT*)&target, value))
        .SetReturn(INT16_MIN);

    ///act
    int16_t return_val = interlocked_exchange_16(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_031: [interlocked_exchange_64 shall call InterlockedExchange64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_032 : [interlocked_exchange_64 shall return the initial value pointed to by target.]*/
TEST_FUNCTION(interlocked_exchange_64_calls_InterlockedExchange64)
{
    ///arrange
    volatile int64_t target;
    InterlockedExchange64(&target, INT64_MIN);
    int64_t value = INT64_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchange64((volatile LONG64*)&target, value))
        .SetReturn(INT64_MIN);

    ///act
    int64_t return_val = interlocked_exchange_64(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_033: [interlocked_exchange_8 shall call InterlockedExchange8 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_034 : [interlocked_exchange_8 shall return the initial value pointed to by target.]*/
TEST_FUNCTION(interlocked_exchange_8_calls_InterlockedExchange8)
{
    ///arrange
    volatile int8_t target;
    InterlockedExchange8((volatile CHAR*)&target, (CHAR)INT8_MIN);
    int8_t value = INT8_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchange8((volatile char*)&target, value))
        .SetReturn(INT8_MIN);

    ///act
    int8_t return_val = interlocked_exchange_8(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int8_t, INT8_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_035: [interlocked_exchange_add shall call InterlockedExchangeAdd from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_036 : [interlocked_exchange_add shall return the initial value of * addend.]*/
TEST_FUNCTION(interlocked_exchange_add_calls_InterlockedExchangeAdd)
{
    ///arrange
    volatile int32_t addend;
    InterlockedExchange((volatile LONG*)&addend, INT32_MIN);
    int32_t value = INT32_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchangeAdd((volatile LONG*)&addend, value))
        .SetReturn(INT32_MIN);

    ///act
    int32_t return_val = interlocked_exchange_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_037: [interlocked_exchange_add_64 shall call InterlockedExchangeAdd64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_038 : [interlocked_exchange_add_64 shall return the initial value of * addend.]*/
TEST_FUNCTION(interlocked_exchange_add_64_calls_InterlockedExchangeAdd64)
{
    ///arrange
    volatile int64_t addend;
    InterlockedExchange64(&addend, INT64_MIN);
    int64_t value = INT64_MAX;
    STRICT_EXPECTED_CALL(mock_InterlockedExchangeAdd64((volatile LONG64*)&addend, value))
        .SetReturn(INT64_MIN);

    ///act
    int64_t return_val = interlocked_exchange_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_039: [interlocked_exchange_pointer shall call InterlockedExchangePointer from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_040 : [interlocked_exchange_pointer shall return the initial address pointed to by the target parameter]*/
TEST_FUNCTION(interlocked_exchange_pointer_calls_InterlockedExchagePointer)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile target = &value1;
    void* value = &value2;
    STRICT_EXPECTED_CALL(mock_InterlockedExchangePointer((PVOID volatile*)&target, value))
        .SetReturn(&value1);

    ///act
    void* return_val = interlocked_exchange_pointer(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_041: [interlocked_increment shall call InterlockedIncrement from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_042 : [interlocked_increment shall return the incremented 32 - bit integer.]*/
TEST_FUNCTION(interlocked_increment_calls_InterlockedIncrement)
{
    ///arrange
    volatile int32_t addend;
    InterlockedExchange((volatile LONG*)&addend, INT32_MAX - 1);
    STRICT_EXPECTED_CALL(mock_InterlockedIncrement((volatile LONG*)&addend))
        .SetReturn(INT32_MAX);

    ///act

    int32_t return_val = interlocked_increment(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_043: [interlocked_increment_16 shall call InterlockedIncrement16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_044 : [interlocked_increment_16 shall return the incremented 16 - bit integer.]*/
TEST_FUNCTION(interlocked_increment_16_calls_InterlockedIncrement16)
{
    ///arrange
    volatile int16_t addend;
    InterlockedExchange16(&addend, INT16_MAX - 1);
    STRICT_EXPECTED_CALL(mock_InterlockedIncrement16((volatile SHORT*)&addend))
        .SetReturn(INT16_MAX);

    ///act

    int16_t return_val = interlocked_increment_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_045: [interlocked_increment_64 shall call InterlockedIncrement64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_046 : [interlocked_increment_64 shall return the incremented 64 - bit integer.]*/
TEST_FUNCTION(interlocked_increment_64_calls_InterlockedIncrement64)
{
    ///arrange
    volatile int64_t addend;
    InterlockedExchange64(&addend, INT64_MAX - 1);
    STRICT_EXPECTED_CALL(mock_InterlockedIncrement64((volatile LONG64*)&addend))
        .SetReturn(INT64_MAX);

    ///act

    int64_t return_val = interlocked_increment_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_047: [interlocked_or shall call InterlockedOr from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_048 : [interlocked_or shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_or_calls_InterlockedOr)
{
    ///arrange
    volatile uint32_t destination;
    InterlockedExchange((volatile LONG*)&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedOr((volatile LONG*)&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_or((volatile int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_049: [interlocked_or_16 shall call InterlockedOr16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_050 : [interlocked_or_16 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_or_16_calls_InterlockedOr16)
{
    ///arrange
    volatile uint16_t destination;
    InterlockedExchange16((volatile int16_t*)&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedOr16((volatile SHORT*)&destination, value))
        .SetReturn((uint16_t)0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_or_16((volatile int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_051: [interlocked_or_64 shall call InterlockedOr64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_052 : [interlocked_or_64 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_or_64_calls_InterlockedOr64)
{
    ///arrange
    volatile uint64_t destination;
    InterlockedExchange64((volatile LONG64*)&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedOr64((volatile LONG64*)&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_or_64((volatile int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_053: [interlocked_or_8 shall call InterlockedOr8 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_054 : [interlocked_or_8 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_or_8_calls_InterlockedOr8)
{
    ///arrange
    volatile uint8_t destination;
    InterlockedExchange8((volatile CHAR*)&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_InterlockedOr8((volatile char*)&destination, value))
        .SetReturn((uint8_t)0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_or_8((volatile int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_055: [interlocked_xor shall call InterlockedXor from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_056 : [interlocked_xor shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_xor_calls_InterlockedXor)
{
    ///arrange
    volatile uint32_t destination;
    InterlockedExchange((volatile LONG*)&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedXor((volatile LONG*)&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_xor((volatile int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_057: [interlocked_xor_16 shall call InterlockedXor16 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_058 : [interlocked_xor_16 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_xor_16_calls_InterlockedXor16)
{
    ///arrange
    volatile uint16_t destination;
    InterlockedExchange16((volatile int16_t*)&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedXor16((volatile SHORT*)&destination, value))
        .SetReturn((uint16_t)0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_xor_16((volatile int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_059: [interlocked_xor_64 shall call InterlockedXor64 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_060 : [interlocked_xor_64 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_xor_64_calls_InterlockedXor64)
{
    ///arrange
    volatile uint64_t destination;
    InterlockedExchange64((volatile LONG64*)&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_InterlockedXor64((volatile LONG64*)&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_xor_64((volatile int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_WIN32_43_061: [interlocked_xor_8 shall call InterlockedXor8 from windows.h.]*/
/*Tests_SRS_INTERLOCKED_WIN32_43_062 : [interlocked_xor_8 shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_xor_8_calls_InterlockedXor8)
{
    ///arrange
    volatile uint8_t destination;
    InterlockedExchange8((volatile CHAR*)&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_InterlockedXor8((volatile char*)&destination, value))
        .SetReturn((uint8_t)0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_xor_8((volatile int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
