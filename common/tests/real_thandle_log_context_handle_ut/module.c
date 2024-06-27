// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "c_logging/log_context.h"

#include "c_pal/thandle_ll.h"
#include "c_pal/thandle_ptr.h"
#include "c_pal/thandle_log_context_handle.h"

#include "module.h"

void function_under_test(void)
{

    LOG_CONTEXT_HANDLE log_context_handle =(LOG_CONTEXT_HANDLE)0x42; // This is a dummy value that is not used in the test.

    /*create a THANDLE out of the LOG_CONTEXT_HANDLE*/
    THANDLE(PTR(LOG_CONTEXT_HANDLE)) thandle = THANDLE_PTR_CREATE_WITH_MOVE(LOG_CONTEXT_HANDLE)(&log_context_handle, NULL); // This is a dummy value that is not used in the test.

    /*here be pretending usage*/

    /*get rid of it*/
    THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(&thandle, NULL);
}
