// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SYNC_H
#define REAL_SYNC_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/sync.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SYNC_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        wait_on_address, \
        wait_on_address_64, \
        wake_by_address_all, \
        wake_by_address_all_64, \
        wake_by_address_single, \
        wake_by_address_single_64 \
)

#ifdef __cplusplus
extern "C" {
#endif


WAIT_ON_ADDRESS_RESULT real_wait_on_address(volatile_atomic int32_t* address, int32_t compare_value, uint32_t timeout_ms);
WAIT_ON_ADDRESS_RESULT real_wait_on_address_64(volatile_atomic int64_t* address, int64_t compare_value, uint32_t timeout_ms);
void real_wake_by_address_all(volatile_atomic int32_t* address);
void real_wake_by_address_all_64(volatile_atomic int64_t* address);
void real_wake_by_address_single(volatile_atomic int32_t* address);
void real_wake_by_address_single_64(volatile_atomic int64_t* address);

#ifdef __cplusplus
}
#endif

#endif // REAL_SYNC_H
