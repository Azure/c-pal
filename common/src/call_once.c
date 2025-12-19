// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

#include "c_pal/call_once.h"

MU_DEFINE_ENUM_STRINGS(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);

/*a weak check to ensure that nobody has the idea to change call_once_t type to int8_t (or other type...  to save some memory for example)*/
static int MU_UNUSED_VAR check_call_once_t_is_the_same_as_volatile_atomic_int32_t_because_we_are_going_to_pass_it_to_wait_on_address_or_wake_by_address_all[sizeof(volatile_atomic int32_t) == sizeof(call_once_t)];

#define CALL_ONCE_STATE_VALUES \
    CALL_ONCE_STATE_NOT_CALLED = CALL_ONCE_NOT_CALLED, \
    CALL_ONCE_STATE_CALLING, \
    CALL_ONCE_STATE_CALLED

MU_DEFINE_ENUM_WITHOUT_INVALID(CALL_ONCE_STATE, CALL_ONCE_STATE_VALUES)

/*returns CALL_ONCE_PROCEED if calling code should do proceed to call the code, any other value = code already called*/
CALL_ONCE_RESULT call_once_begin(call_once_t* state)
{
    CALL_ONCE_RESULT result = CALL_ONCE_ALREADY_CALLED; /*optimistically initialize*/
    
    int32_t state_local;

    /* 0 - (CALL_ONCE_NOT_CALLED) = not called*/
    /* 1 - (CALL_ONCE_NOT_CALLED+1) calling... */
    /* 2 - (CALL_ONCE_NOT_CALLED+2) already called*/

    /*Codes_SRS_CALL_ONCE_02_001: [ call_once_begin shall use interlocked_compare_exchange(state, 1, 0) to determine if user has alredy indicated that the init code was executed with success. ]*/
    /*Codes_SRS_CALL_ONCE_02_002: [ If interlocked_compare_exchange returns 2 then call_once_begin shall return CALL_ONCE_ALREADY_CALLED. ]*/
    while ((state_local = interlocked_compare_exchange(state, CALL_ONCE_STATE_CALLING, CALL_ONCE_STATE_NOT_CALLED)) != CALL_ONCE_STATE_CALLED)
    {
        if (state_local == CALL_ONCE_STATE_NOT_CALLED) /*it was not initialized, now it is "1" (initializing)*/
        {
            /*Codes_SRS_CALL_ONCE_02_004: [ If interlocked_compare_exchange returns 0 then call_once_begin shall return CALL_ONCE_PROCEED. ]*/
            /*only 1 thread can get here, (the current one) so letting it proceed to initialize*/
            result = CALL_ONCE_PROCEED;
            break;
        }
        else
        {
            /*Codes_SRS_CALL_ONCE_02_003: [ If interlocked_compare_exchange returns 1 then call_once_begin shall call wait_on_address(state) with timeout UINT32_MAX and call again interlocked_compare_exchange(state, 1, 0). ]*/
            /*state cannot be "2" because while condition, so that means state is "1", and that means, need to wait until it is not 1*/
            if (wait_on_address(state, CALL_ONCE_STATE_CALLING, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
            {
                /*look the other way and retry*/
                LogError("failure in wait_on_address, var=%p", state);
            }
            else
            {
                /*state is not 1 here, but it can be 0 or 2, so a reevaluation is needed*/
            }
        }
    }
    return result;
}

void call_once_end(call_once_t* state, bool success)
{
    /*Codes_SRS_CALL_ONCE_02_005: [ If success is true then call_once_end shall call interlocked_exchange setting state to CALL_ONCE_CALLED and shall call wake_by_address_all(state). ]*/
    /*Codes_SRS_CALL_ONCE_02_006: [ If success is false then call_once_end shall call interlocked_exchange setting state to CALL_ONCE_NOT_CALLED and shall call wake_by_address_all(state). ]*/
    (void)interlocked_exchange(state, success ? CALL_ONCE_STATE_CALLED : CALL_ONCE_STATE_NOT_CALLED);
    wake_by_address_all(state);
}




