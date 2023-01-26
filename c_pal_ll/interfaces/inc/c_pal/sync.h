// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SYNC_H
#define SYNC_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "c_pal/interlocked.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WAIT_ON_ADDRESS_RESULT_VALUES \
    WAIT_ON_ADDRESS_OK, \
    WAIT_ON_ADDRESS_ERROR, \
    WAIT_ON_ADDRESS_TIMEOUT

MU_DEFINE_ENUM(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address);

#ifdef __cplusplus
}
#endif

#endif
