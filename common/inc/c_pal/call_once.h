// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef CALL_ONCE_H
#define CALL_ONCE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"
#include "c_pal/interlocked.h"

typedef volatile_atomic int32_t call_once_t;

#define CALL_ONCE_NOT_CALLED 0 /*to only be used in static initialization, rest of initializations need to use interlocked_exchange*/

#define CALL_ONCE_RESULT_VALUES \
    CALL_ONCE_PROCEED, \
    CALL_ONCE_ALREADY_CALLED \

MU_DEFINE_ENUM(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES)

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, CALL_ONCE_RESULT, call_once_begin, call_once_t*, state);
    MOCKABLE_FUNCTION(, void, call_once_end, call_once_t*, state, bool, success);

#ifdef __cplusplus
}
#endif

#endif // CALL_ONCE_H

