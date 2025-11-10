// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "windows.h"

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"


MU_DEFINE_ENUM_STRINGS(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES)

WAIT_ON_ADDRESS_RESULT wait_on_address(volatile_atomic int32_t* address, int32_t compare_value, uint32_t timeout_ms)
{
    WAIT_ON_ADDRESS_RESULT result;
    /* Codes_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress to wait on the value at 32-bit address to be different than compare_value for timeout_ms milliseconds. ] */
    if (WaitOnAddress(address, &compare_value, sizeof(int32_t), timeout_ms) != TRUE)
    {
        if (GetLastError() == ERROR_TIMEOUT)
        {
            /* Codes_SRS_SYNC_WIN32_24_001: [ If WaitOnAddress fails due to timeout, wait_on_address shall fail and return WAIT_ON_ADDRESS_TIMEOUT. ] */
            result = WAIT_ON_ADDRESS_TIMEOUT;
        }
        else
        {
            LogLastError("failure in WaitOnAddress(address=%p, &compare_value=%p, address_size=%zu, timeout_ms=%" PRIu32 ")",
                address, &compare_value, sizeof(int32_t), timeout_ms);
            /* Codes_SRS_SYNC_WIN32_24_002: [ If WaitOnAddress fails due to any other reason, wait_on_address shall fail and return WAIT_ON_ADDRESS_ERROR. ] */
            result = WAIT_ON_ADDRESS_ERROR;
        }
    }
    else
    {
        /* Codes_SRS_SYNC_WIN32_24_003: [ If WaitOnAddress succeeds, wait_on_address shall return WAIT_ON_ADDRESS_OK. ] */
        result = WAIT_ON_ADDRESS_OK;
    }
    return result;
}

WAIT_ON_ADDRESS_RESULT wait_on_address_64(volatile_atomic int64_t* address, int64_t compare_value, uint32_t timeout_ms)
{
    WAIT_ON_ADDRESS_RESULT result;
    /* Codes_SRS_SYNC_WIN32_05_001: [ wait_on_address_64 shall call WaitOnAddress to wait on the value at 64-bit address to be different than compare_value for timeout_ms milliseconds. ] */
    if (WaitOnAddress(address, &compare_value, sizeof(int64_t), timeout_ms) != TRUE)
    {
        if (GetLastError() == ERROR_TIMEOUT)
        {
            /* Codes_SRS_SYNC_WIN32_05_002: [ If WaitOnAddress fails due to timeout, wait_on_address_64 shall fail and return WAIT_ON_ADDRESS_TIMEOUT. ] */
            result = WAIT_ON_ADDRESS_TIMEOUT;
        }
        else
        {
            LogLastError("failure in WaitOnAddress(address=%p, &compare_value=%p, address_size=%zu, timeout_ms=%" PRIu32 ")",
                address, &compare_value, sizeof(int64_t), timeout_ms);
            /* Codes_SRS_SYNC_WIN32_05_003: [ If WaitOnAddress fails due to any other reason, wait_on_address_64 shall fail and return WAIT_ON_ADDRESS_ERROR. ] */
            result = WAIT_ON_ADDRESS_ERROR;
        }
    }
    else
    {
        /* Codes_SRS_SYNC_WIN32_05_004: [ If WaitOnAddress succeeds, wait_on_address_64 shall return WAIT_ON_ADDRESS_OK. ] */
        result = WAIT_ON_ADDRESS_OK;
    }
    return result;
}

void wake_by_address_all(volatile_atomic int32_t* address)
{
    /* Codes_SRS_SYNC_WIN32_43_003: [ wake_by_address_all shall call WakeByAddressAll to notify all listeners waiting on the 32-bit address. ] */
    WakeByAddressAll((PVOID)address);
}

void wake_by_address_all_64(volatile_atomic int64_t* address)
{
    /* Codes_SRS_SYNC_WIN32_05_005: [ wake_by_address_all_64 shall call WakeByAddressAll to notify all listeners waiting on the 64-bit address. ] */
    WakeByAddressAll((PVOID)address);
}

void wake_by_address_single(volatile_atomic int32_t* address)
{
    /* Codes_SRS_SYNC_WIN32_43_004: [ wake_by_address_single shall call WakeByAddressSingle to notify a single listeners waiting on the 32-bit address. ] */
    WakeByAddressSingle((PVOID)address);
}

void wake_by_address_single_64(volatile_atomic int64_t* address)
{
    /* Codes_SRS_SYNC_WIN32_05_006: [ wake_by_address_single_64 shall call WakeByAddressSingle to notify a single listeners waiting on the 64-bit address. ] */
    WakeByAddressSingle((PVOID)address);
}

