// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdbool.h>

#include "windows.h"

#include "azure_c_logging/xlogging.h"

#include "azure_c_pal/interlocked.h"
#include "azure_c_pal/interlocked_hl.h"

#include "azure_c_pal/call_once.h"

MU_DEFINE_ENUM_STRINGS(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);

/*returns CALL_ONCE_PROCEED if calling code should do proceed to call the code, any other value = code already called*/
CALL_ONCE_RESULT call_once_begin(volatile_atomic int32_t* state)
{
    CALL_ONCE_RESULT result = CALL_ONCE_ALREADY_CALLED; /*optimistically initialize*/
    
    int32_t state_local;

    /* 0 - not called*/
    /* 1 - calling... */
    /* 2 - already called*/

    /*Codes_SRS_CALL_ONCE_02_001: [ call_once_begin shall use interlocked_compare_exchange(state, 1, 0) to determine if user has alredy indicated that the init code was executed with success. ]*/
    /*Codes_SRS_CALL_ONCE_02_002: [ If interlocked_compare_exchange returns 2 then call_once_begin shall return CALL_ONCE_ALREADY_CALLED. ]*/
    while ((state_local = interlocked_compare_exchange(state, 1, 0)) != 2)
    {
        if (state_local == 0) /*it was not initialized, now it is "1" (initializing)*/
        {
            /*Codes_SRS_CALL_ONCE_02_004: [ If interlocked_compare_exchange returns 0 then call_once_begin shall return CALL_ONCE_PROCEED. ]*/
            /*only 1 thread can get here, (the current one) so letting it proceed to initialize*/
            result = CALL_ONCE_PROCEED;
            break;
        }
        else
        {
            /*Codes_SRS_CALL_ONCE_02_003: [ If interlocked_compare_exchange returns 1 then call_once_begin shall call InterlockedHL_WaitForNotValue(state, 1, INFINITE) and call again interlocked_compare_exchange(state, 1, 0). ]*/
            /*state cannot be "2" because while condition, so that means state is "1", and that means, need to wait until it is not 1*/
            if (InterlockedHL_WaitForNotValue((LONG*)state, 1, INFINITE) != INTERLOCKED_HL_OK)
            {
                /*look the other way and retry*/
                LogError("failure in InterlockedHL_WaitForValue, var=%p", state);
            }
            else
            {
                /*state is not 1 here, but it can be 0 or 2, so a reevaluation is needed*/
            }
        }
    }
    return result;
}

void call_once_end(volatile_atomic int32_t* state, bool success)
{
    /*Codes_SRS_CALL_ONCE_02_005: [ If success is true then call_once_end shall call InterlockedHL_SetAndWakeAll(state, 2). ]*/
    /*Codes_SRS_CALL_ONCE_02_006: [ If success is false then call_once_end shall call InterlockedHL_SetAndWakeAll(state, 0). ]*/
    if (InterlockedHL_SetAndWakeAll((LONG*)state, success ? 2 : 0) != INTERLOCKED_HL_OK)
    {
        LogError("failure in InterlockedHL_SetAndWake((LONG*)state=%p, success ? 2 : 0)", state);
    }
}




