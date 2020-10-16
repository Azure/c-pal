// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SYNC_H
#define REAL_SYNC_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_pal/srw_lock.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SYNC_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        wait_on_address, \
        wake_by_address_all, \
        wake_by_address_single \
)

#ifdef __cplusplus
extern "C" {
#endif


bool real_wait_on_address(volatile_atomic int32_t* address, int32_t compare_value, uint32_t timeout_ms);
void real_wake_by_address_all(volatile_atomic int32_t* address);
void real_wake_by_address_single(volatile_atomic int32_t* address);

#ifdef __cplusplus
}
#endif

#endif // REAL_SYNC_H
