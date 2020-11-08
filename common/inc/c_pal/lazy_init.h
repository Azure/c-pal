// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef LAZY_INIT_H
#define LAZY_INIT_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"
#include "c_pal/interlocked.h"
#include "c_pal/call_once.h"

#define LAZY_INIT_NOT_DONE CALL_ONCE_NOT_CALLED /*to only be used in static initialization, rest of initializations need to use interlocked_exchange*/

#define LAZY_INIT_RESULT_VALUES \
    LAZY_INIT_OK, \
    LAZY_INIT_ERROR

MU_DEFINE_ENUM(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES)

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    typedef int (*LAZY_INIT_FUNCTION)(void* params);

    MOCKABLE_FUNCTION(, LAZY_INIT_RESULT, lazy_init, call_once_t*, lazy, LAZY_INIT_FUNCTION, do_init, void*, init_params);

#ifdef __cplusplus
}
#endif

#endif // LAZY_INIT_H

