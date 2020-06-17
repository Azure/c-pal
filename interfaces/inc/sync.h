// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SYNC_H
#define SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifndef volatile_atomic
#define volatile_atomic volatile
#else
#define volatile_atomic volatile _Atomic
#endif
#endif

MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout);
MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address);


#ifdef __cplusplus
}
#endif
#endif