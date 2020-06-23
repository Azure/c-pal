// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "umock_c/umock_c_prod.h"


#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, BOOL, mock_WaitOnAddress, volatile VOID*, Address, PVOID, CompareAddress, SIZE_T, AddressSize, DWORD, dwMilliseconds);
MOCKABLE_FUNCTION(, void, mock_WakeByAddressAll, PVOID, address);
MOCKABLE_FUNCTION(, void, mock_WakeByAddressSingle, PVOID, address);

#ifdef __cplusplus
}
#endif
