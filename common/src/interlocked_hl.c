// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stddef.h>                 // for NULL

#include "umock_c/umock_c_prod.h"

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/interlocked_hl.h"

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, int64_t volatile_atomic*, Addend, int64_t, Ceiling, int64_t, Value, int64_t*, originalAddend)
{
    INTERLOCKED_HL_RESULT result;
    /*Codes_SRS_INTERLOCKED_HL_02_001: [ If Addend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    /*Codes_SRS_INTERLOCKED_HL_02_006: [ If originalAddend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (
        (Addend == NULL) ||
        (originalAddend == NULL)
        )
    {
        LogError("invalid arguments int64_t volatile_atomic* Addend=%p, int64_t Ceiling=%" PRId64 ", int64_t Value=%" PRId64 ", int64_t* originalAddend=%p",
            Addend, Ceiling, Value, originalAddend);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        while(1)
        {
            /*checking if Addend + Value is representable*/
            int64_t addend_copy;
            int64_t expected_operation_result;

            addend_copy = interlocked_add_64(Addend, 0);

            /*Codes_SRS_INTERLOCKED_HL_02_003: [ If Addend + Value would overflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
            if (
                ((addend_copy >= 0) && (Value > INT64_MAX - addend_copy)) ||
                ((addend_copy < 0) && (Value < INT64_MIN - addend_copy))
                )
            {
                /*Codes_SRS_INTERLOCKED_HL_02_002: [ If Addend + Value would underflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
                /*would overflow*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
            else
            {
                expected_operation_result = addend_copy + Value;
                /*Codes_SRS_INTERLOCKED_HL_02_004: [ If Addend + Value would be greater than Ceiling then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
                if (expected_operation_result > Ceiling)
                {
                    result = INTERLOCKED_HL_ERROR;
                    break;
                }
                else
                {
                    /*Codes_SRS_INTERLOCKED_HL_02_005: [ Otherwise, InterlockedHL_Add64WithCeiling shall atomically write in Addend the sum of Addend and Value, succeed and return INTERLOCKED_HL_OK. ]*/
                    if (interlocked_compare_exchange_64(Addend, expected_operation_result, addend_copy) == addend_copy)
                    {
                        /*Codes_SRS_INTERLOCKED_HL_02_007: [ In all failure cases InterlockedHL_Add64WithCeiling shall not modify Addend or originalAddend*/
                        *originalAddend = addend_copy;
                        result = INTERLOCKED_HL_OK;
                        break;
                    }
                    else
                    {
                        /*go back to while(1)*/
                    }
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)
{
    int32_t volatile_atomic* address_to_check = address;
    int32_t value_to_wait = value;
    uint32_t timeout_ms = milliseconds;
    INTERLOCKED_HL_RESULT result = INTERLOCKED_HL_ERROR;

    if (address_to_check == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_01_002: [ If address_to_check is NULL, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ] */
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int32_t current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_01_007: [ When wait_on_address succeeds, the value at address shall be compared to the target value passed in value by using interlocked_add. ] */
            current_value = interlocked_add(address_to_check, 0);
            if (current_value == value_to_wait)
            {
                /* Codes_SRS_INTERLOCKED_HL_01_003: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ] */
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_01_004: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue shall wait until the value at address_to_check changes using wait_on_address. ]*/
            WAIT_ON_ADDRESS_RESULT wait_result = wait_on_address(address_to_check, current_value, timeout_ms);
            if (wait_result == WAIT_ON_ADDRESS_OK)
            {
                /* Codes_SRS_INTERLOCKED_HL_01_007: [ When wait_on_address succeeds, InterlockedHL_WaitForValue shall again compare the value at address_to_check with value_to_wait. ] */
                // Loop back to compare current_value and value_to_wait
                continue;
            }
            else if (wait_result == WAIT_ON_ADDRESS_TIMEOUT)
            {
                /* Codes_SRS_INTERLOCKED_HL_11_001: [ If wait_on_address hits the timeout specified in timeout_ms, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
                result = INTERLOCKED_HL_TIMEOUT;
                break;
            }
            else
            {
                LogError("Failure in InterlockedHL_WaitForValue(address=%p, value=%" PRId32 ", timeout_ms=%" PRIu32 ") current_value=%" PRId32 " result: %" PRI_MU_ENUM " while calling wait_on_address_64.",
                    address_to_check, value_to_wait, timeout_ms, current_value, MU_ENUM_VALUE(WAIT_ON_ADDRESS_RESULT, wait_result));
                /* Codes_SRS_INTERLOCKED_HL_01_006: [ If wait_on_address fails, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue64, int64_t volatile_atomic*, address, int64_t, value, uint32_t, milliseconds)
{
    int64_t volatile_atomic* address_to_check = address;
    int64_t value_to_wait = value;
    uint32_t timeout_ms = milliseconds;
    INTERLOCKED_HL_RESULT result = INTERLOCKED_HL_ERROR;

    if (address_to_check == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_05_001: [ If address_to_check is NULL, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int64_t current_value;
        do
        {
            current_value = interlocked_add_64(address_to_check, 0);
            if (current_value == value_to_wait)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_002: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForValue64 shall return INTERLOCKED_HL_OK. ] */
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_05_003: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ] */
            WAIT_ON_ADDRESS_RESULT wait_result = wait_on_address_64(address_to_check, current_value, timeout_ms);
            if (wait_result == WAIT_ON_ADDRESS_OK)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_004: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForValue64 shall again compare the value at address_to_check with value_to_wait. */
                // Loop back to check current_value and value_to_wait
                continue;
            }
            else if (wait_result == WAIT_ON_ADDRESS_TIMEOUT)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_005: [ If wait_on_address_64 hits the timeout specified in timeout_ms, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
                result = INTERLOCKED_HL_TIMEOUT;
                break;
            }
            else
            {
                LogError("Failure in InterlockedHL_WaitForValue64(address=%p, value=%" PRId64 ", timeout_ms=%" PRIu32 ") current_value=%" PRId64 " result: %" PRI_MU_ENUM " while calling wait_on_address_64.",
                    address_to_check, value_to_wait, timeout_ms, current_value, MU_ENUM_VALUE(WAIT_ON_ADDRESS_RESULT, wait_result));
                /* Codes_SRS_INTERLOCKED_HL_05_006: [ If wait_on_address_64 fails, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)
{
    int32_t volatile_atomic* address_to_check = address;
    int32_t value_to_wait = value;
    uint32_t timeout_ms = milliseconds;
    INTERLOCKED_HL_RESULT result = INTERLOCKED_HL_ERROR;

    if (address_to_check == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_42_001: [ If address_to_check is NULL, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%d, uint32_t milliseconds=%u",
            address_to_check, value_to_wait, timeout_ms);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int32_t current_value;

        do
        {
            current_value = interlocked_add(address, 0);
            if (current_value != value)
            {
                /* Codes_SRS_INTERLOCKED_HL_42_002: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_42_003: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue shall wait until the value at address_to_check changes by using wait_on_address. ]*/
            WAIT_ON_ADDRESS_RESULT wait_result = wait_on_address(address, current_value, milliseconds);
            if (wait_result == WAIT_ON_ADDRESS_OK)
            {
                /* Codes_SRS_INTERLOCKED_HL_42_005: [When wait_on_address succeeds, InterlockedHL_WaitForNotValue shall again compare the value at address_to_check with value_to_wait.]*/
                // Loop back to check current_value and value_to_wait
                continue;
            }
            else if (wait_result == WAIT_ON_ADDRESS_TIMEOUT)
            {
                /* Codes_SRS_INTERLOCKED_HL_11_002: [ If wait_on_address hits the timeout specified in timeout_ms, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
                result = INTERLOCKED_HL_TIMEOUT;
                break;
            }
            else
            {
                LogError("Failure in InterlockedHL_WaitForNotValue(address=%p, value=%" PRId32 ", timeout_ms=%" PRIu32 ") current_value=%" PRId32 " result: %" PRI_MU_ENUM " while calling wait_on_address.",
                    address_to_check, value_to_wait, timeout_ms, current_value, MU_ENUM_VALUE(WAIT_ON_ADDRESS_RESULT, wait_result));
                /* Codes_SRS_INTERLOCKED_HL_42_007: [ If wait_on_address fails, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ] */
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue64, int64_t volatile_atomic*, address, int64_t, value, uint32_t, milliseconds)
{
    int64_t volatile_atomic* address_to_check = address;
    int64_t value_to_wait = value;
    uint32_t timeout_ms = milliseconds;
    INTERLOCKED_HL_RESULT result = INTERLOCKED_HL_ERROR;

    if (address_to_check == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_05_007: [ If address_to_check is NULL, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int64_t current_value;
        do
        {
            current_value = interlocked_add_64(address_to_check, 0);
            if (current_value != value_to_wait)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_008: [ If the value at address_to_check is not equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall return INTERLOCKED_HL_OK. ] */
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_05_009: [ If the value at address_to_check is equal to value_to_wait, InterlockedHL_WaitForNotValue64 shall wait until the value at address_to_check changes using wait_on_address_64. ]*/
            WAIT_ON_ADDRESS_RESULT wait_result = wait_on_address_64(address_to_check, current_value, timeout_ms);
            if (wait_result == WAIT_ON_ADDRESS_OK)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_010: [ When wait_on_address_64 succeeds, InterlockedHL_WaitForNotValue64 shall again compare the value at address_to_check with value_to_wait. ] */
                // Loop back to compare current_value and value_to_wait
                continue;
            }
            else if (wait_result == WAIT_ON_ADDRESS_TIMEOUT)
            {
                /* Codes_SRS_INTERLOCKED_HL_05_011: [ If wait_on_address_64 hits the timeout specified in timeout_ms, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_TIMEOUT. ] */
                result = INTERLOCKED_HL_TIMEOUT;
                break;
            }
            else
            {
                LogError("Failure in InterlockedHL_WaitForNotValue(address=%p, value=%" PRId64 ", timeout_ms=%" PRIu32 ") current_value=%" PRId64 " result: %" PRI_MU_ENUM " while calling wait_on_address_64.",
                    address_to_check, value_to_wait, timeout_ms, current_value, MU_ENUM_VALUE(WAIT_ON_ADDRESS_RESULT, wait_result));
                /* Codes_SRS_INTERLOCKED_HL_05_012: [ If wait_on_address_64 fails, InterlockedHL_WaitForNotValue64 shall fail and return INTERLOCKED_HL_ERROR. ] */
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchangeIf, int32_t volatile_atomic*, target, int32_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_IF, compare, int32_t*, original_target)
{
    INTERLOCKED_HL_RESULT result;
    if (
        /* Codes_SRS_INTERLOCKED_HL_01_009: [ If target is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (target == NULL) ||
        /* Codes_SRS_INTERLOCKED_HL_01_010: [ If compare is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (compare == NULL) ||
        /* Codes_SRS_INTERLOCKED_HL_01_011: [ If original_target is NULL then InterlockedHL_CompareExchangeIf shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (original_target == NULL)
        )
    {
        LogError("invalid arguments int64_t volatile_atomic* target=%p, int32_t exchange=%" PRId32 ", INTERLOCKED_COMPARE_EXCHANGE_IF compare=%p original_target=%p",
            target, exchange, compare, original_target);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /* Codes_SRS_INTERLOCKED_HL_01_012: [ InterlockedHL_CompareExchangeIf shall acquire the initial value of target. ]*/
        int32_t copyOfTarget = interlocked_add(target, 0);

        /* Codes_SRS_INTERLOCKED_HL_01_013: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchangeIf shall exchange target with exchange. ]*/
        if (compare(copyOfTarget, exchange))
        {
            if (interlocked_compare_exchange(target, exchange, copyOfTarget) == copyOfTarget)
            {
                /* Codes_SRS_INTERLOCKED_HL_01_015: [ If target did not change meanwhile then InterlockedHL_CompareExchangeIf shall return INTERLOCKED_HL_OK and shall peform the exchange of values. ]*/
                result = INTERLOCKED_HL_OK;
            }
            else
            {
                /* Codes_SRS_INTERLOCKED_HL_01_014: [ If target changed meanwhile then InterlockedHL_CompareExchangeIf shall return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
                result = INTERLOCKED_HL_CHANGED;
            }
        }
        else
        {
            /* Codes_SRS_INTERLOCKED_HL_01_017: [ If compare returns false then  InterlockedHL_CompareExchangeIf shall not perform any exchanges and return INTERLOCKED_HL_OK. ]*/
            result = INTERLOCKED_HL_OK;
        }

        /* Codes_SRS_INTERLOCKED_HL_01_016: [ original_target shall be set to the original value of target. ]*/
        *original_target = copyOfTarget;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, int64_t volatile_atomic*, target, int64_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, int64_t*, original_target)
{
    INTERLOCKED_HL_RESULT result;
    if (
        /*Codes_SRS_INTERLOCKED_HL_02_008: [ If target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (target == NULL) ||
        /*Codes_SRS_INTERLOCKED_HL_02_009: [ If compare is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (compare == NULL) ||
        /*Codes_SRS_INTERLOCKED_HL_02_010: [ If original_target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (original_target == NULL)
        )
    {
        LogError("invalid arguments int64_t volatile_atomic* target=%p, int64_t exchange=%" PRId64 ", INTERLOCKED_COMPARE_EXCHANGE_64_IF compare=%p original_target=%p",
            target, exchange, compare, original_target);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
        int64_t copyOfTarget = interlocked_add_64(target, 0);

        /*Codes_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ]*/
        if (compare(copyOfTarget, exchange))
        {
            /*Codes_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
            /*Codes_SRS_INTERLOCKED_HL_02_014: [ If target did not change meanwhile then InterlockedHL_CompareExchange64If shall return INTERLOCKED_HL_OK and shall peform the exchange of values. ]*/
            if (interlocked_compare_exchange_64(target, exchange, copyOfTarget) == copyOfTarget)
            {
                result = INTERLOCKED_HL_OK;
            }
            else
            {
                result = INTERLOCKED_HL_CHANGED;
            }
        }
        else
        {
            /*Codes_SRS_INTERLOCKED_HL_02_015: [ If compare returns false then InterlockedHL_CompareExchange64If shall not perform any exchanges and return INTERLOCKED_HL_OK. ]*/
            result = INTERLOCKED_HL_OK;
        }
        /*Codes_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ]*/
        *original_target = copyOfTarget;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, int32_t volatile_atomic*, address, int32_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_02_020: [ If address is NULL then InterlockedHL_SetAndWake shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%" PRId32 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_017: [ InterlockedHL_SetAndWake shall set address to value. ]*/
        (void)interlocked_exchange(address, value);

        /*Codes_SRS_INTERLOCKED_HL_02_018: [ InterlockedHL_SetAndWake shall call wake_by_address_single. ]*/
        wake_by_address_single(address);

        /*Codes_SRS_INTERLOCKED_HL_02_019: [ InterlockedHL_SetAndWake shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake64, int64_t volatile_atomic*, address, int64_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_05_013: [ If address is NULL then InterlockedHL_SetAndWake64 shall fail and return INTERLOCKED_HL_ERROR. ] */
        LogError("invalid arguments int64_t volatile_atomic* address=%p, int64_t value=%" PRId64 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /* Codes_SRS_INTERLOCKED_HL_05_014: [ InterlockedHL_SetAndWake64 shall set value at address. ] */
        (void)interlocked_exchange_64(address, value);

        /* Codes_SRS_INTERLOCKED_HL_05_015: [ InterlockedHL_SetAndWake64 shall wake a single thread listening on address. ] */
        wake_by_address_single_64(address);

        /* Codes_SRS_INTERLOCKED_HL_05_016: [ InterlockedHL_SetAndWake64 shall succeed and return INTERLOCKED_HL_OK. ] */
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll, int32_t volatile_atomic*, address, int32_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_02_028: [ If address is NULL then InterlockedHL_SetAndWakeAll shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%" PRId32 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_029: [ InterlockedHL_SetAndWakeAll shall set address to value. ]*/
        (void)interlocked_exchange(address, value);

        /*Codes_SRS_INTERLOCKED_HL_02_030: [ InterlockedHL_SetAndWakeAll shall call wake_by_address_all. ]*/
        wake_by_address_all(address);

        /*Codes_SRS_INTERLOCKED_HL_02_031: [ InterlockedHL_SetAndWakeAll shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll64, int64_t volatile_atomic*, address, int64_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_05_017: [ If address is NULL then InterlockedHL_SetAndWakeAll64 shall fail and return INTERLOCKED_HL_ERROR. ] */
        LogError("invalid arguments int64_t volatile_atomic* address=%p, int64_t value=%" PRId64 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /* Codes_SRS_INTERLOCKED_HL_05_018: [ InterlockedHL_SetAndWakeAll64 shall set value at address. ] */
        (void)interlocked_exchange_64(address, value);

        /* Codes_SRS_INTERLOCKED_HL_05_019: [ InterlockedHL_SetAndWakeAll64 shall wake all threads listening on address. ] */
        wake_by_address_all_64(address);

        /* Codes_SRS_INTERLOCKED_HL_05_020: [ InterlockedHL_SetAndWakeAll64 shall succeed and return INTERLOCKED_HL_OK. ] */
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake, int32_t volatile_atomic*, address)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_44_001: [ If address is NULL then InterlockedHL_DecrementAndWake shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p", address);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_44_002: [ InterlockedHL_DecrementAndWake shall decrement the value at address by 1. ]*/
        (void)interlocked_decrement(address);

        /*Codes_SRS_INTERLOCKED_HL_44_003: [ InterlockedHL_DecrementAndWake shall call wake_by_address_single. ]*/
        wake_by_address_single(address);

        /*Codes_SRS_INTERLOCKED_HL_44_004: [ InterlockedHL_DecrementAndWake shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_DecrementAndWake64, int64_t volatile_atomic*, address)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /* Codes_SRS_INTERLOCKED_HL_05_021: [ If address is NULL then InterlockedHL_DecrementAndWake64 shall fail and return INTERLOCKED_HL_ERROR. ] */
        LogError("invalid arguments int32_t volatile_atomic* address=%p", address);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /* Codes_SRS_INTERLOCKED_HL_05_022: [ InterlockedHL_DecrementAndWake64 shall decrement the value at address by 1. ] */
        (void)interlocked_decrement_64(address);

        /* Codes_SRS_INTERLOCKED_HL_05_023: [ InterlockedHL_DecrementAndWake64 shall wake a single thread listening on address. ] */
        wake_by_address_single_64(address);

        /* Codes_SRS_INTERLOCKED_HL_05_024: [ InterlockedHL_DecrementAndWake64 shall succeed and return INTERLOCKED_HL_OK. ] */
        result = INTERLOCKED_HL_OK;
    }
    return result;

}
