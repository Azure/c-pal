// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"  // for MU_COUNT_ARG_0, MU_DISPATCH_EMP...

#include "c_logging/logger.h"

#include "c_pal/call_once.h"

#include "c_pal/lazy_init.h"

MU_DEFINE_ENUM_STRINGS(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES)

LAZY_INIT_RESULT lazy_init(call_once_t* lazy, LAZY_INIT_FUNCTION do_init, void* init_params)
{
    LAZY_INIT_RESULT result;
    if (
        /*Codes_SRS_LAZY_INIT_02_001: [ If lazy is NULL then lazy_init shall fail and return LAZY_INIT_ERROR. ]*/
        (lazy == NULL) ||
        /*Codes_SRS_LAZY_INIT_02_002: [ If do_init is NULL then lazy_init shall fail and return LAZY_INIT_ERROR. ]*/
        (do_init == NULL)
        )
    {
        LogError("invalid arguments call_once_t* lazy=%p, LAZY_INIT_FUNCTION do_init=%p, void* init_params=%p",
            lazy, do_init, init_params);
        result = LAZY_INIT_ERROR;
    }
    else
    {
        /*Codes_SRS_LAZY_INIT_02_003: [ lazy_init shall call call_once_begin(lazy). ]*/
        if (call_once_begin(lazy) == CALL_ONCE_PROCEED)
        {
            /*Codes_SRS_LAZY_INIT_02_005: [ If call_once_begin returns CALL_ONCE_PROCEED then lazy_init shall call do_init(init_params). ]*/
            if (do_init(init_params) != 0)
            {
                /*Codes_SRS_LAZY_INIT_02_007: [ If do_init returns different than 0 then lazy_init shall call call_once_end(lazy, false), fail and return LAZY_INIT_ERROR. ]*/
                LogError("failure in do_init(init_params=%p)", init_params);
                call_once_end(lazy, false);
                result = LAZY_INIT_ERROR;
            }
            else
            {
                /*Codes_SRS_LAZY_INIT_02_006: [ If do_init returns 0 then lazy_init shall call call_once_end(lazy, true), succeed and return LAZY_INIT_OK. ]*/
                call_once_end(lazy, true);
                result = LAZY_INIT_OK;
            }
        }
        else
        {
            /*Codes_SRS_LAZY_INIT_02_004: [ If call_once_begin returns CALL_ONCE_ALREADY_CALLED then lazy_init shall succeed and return LAZY_INIT_OK. ]*/
            result = LAZY_INIT_OK;
        }
    }
    return result;
}
