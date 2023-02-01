// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "c_logging/xlogging.h"

#include "windows.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

#include "umock_c/umock_c_prod.h"     // for IMPLEMENT_MOCKABLE_FUNCTION

IMPLEMENT_MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms)
{
    WAIT_ON_ADDRESS_RESULT result;
    /*Codes_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress from windows.h with address as Address, a pointer to the value compare_value as CompareAddress, 4 as AddressSize and timeout_ms as dwMilliseconds. ]*/
    if (WaitOnAddress(address, &compare_value, sizeof(int32_t), timeout_ms) != TRUE)
    {
        if (GetLastError() == ERROR_TIMEOUT)
        {
            /* Codes_SRS_SYNC_WIN32_24_001: [ If WaitOnAddress fails due to timeout, wait_on_address shall fail and return WAIT_ON_ADDRESS_TIMEOUT. ]*/
            result = WAIT_ON_ADDRESS_TIMEOUT;
        }
        else
        {
            LogLastError("failure in WaitOnAddress(address=%p, &compare_value=%p, address_size=%zu, timeout_ms=%" PRId32 ")",
                address, &compare_value, sizeof(int32_t), timeout_ms);
            /* Codes_SRS_SYNC_WIN32_24_002: [ If WaitOnAddress fails due to any other reason, wait_on_address shall fail and return WAIT_ON_ADDRESS_ERROR. ]*/
            result = WAIT_ON_ADDRESS_ERROR;
        }
    }
    else
    {
        /* Codes_SRS_SYNC_WIN32_24_003: [ If WaitOnAddress succeeds, wait_on_address shall return WAIT_ON_ADDRESS_OK. ]*/
        result = WAIT_ON_ADDRESS_OK;
    }
    return result;
}
IMPLEMENT_MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address)
{
    /*Codes_SRS_SYNC_WIN32_43_003: [ wake_by_address_all shall call WakeByAddressAll from windows.h with address as Address. ]*/
    WakeByAddressAll((PVOID)address);
}
IMPLEMENT_MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address)
{
    /*Codes_SRS_SYNC_WIN32_43_004: [ wake_by_address_single shall call WakeByAddressSingle from windows.h with address as Address. ]*/
    WakeByAddressSingle((PVOID)address);
}
