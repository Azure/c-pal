// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "sync.h"

IMPLEMENT_MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout_ms)
{
    /*Codes_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress from windows.h with address as Address, compare_address as CompareAddress, 4 as AddressSize and timeout_ms as dwMilliseconds. ]*/
    /*Codes_SRS_SYNC_WIN32_43_002: [ wait_on_address shall return the return value of WaitOnAddress ]*/
    return WaitOnAddress((volatile VOID*) address, (PVOID)compare_address, (SIZE_T)4, (DWORD)timeout_ms);
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
