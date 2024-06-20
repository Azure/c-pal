// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "c_pal/sm.h"
#include "c_pal/thandle_ll.h"
#include "c_pal/thandle_ptr.h"
#include "c_pal/thandle_sm_handle.h"

#include "module.h"

void function_under_test(void)
{

    SM_HANDLE sm_handle =(SM_HANDLE)0x42; // This is a dummy value that is not used in the test.

    /*create a THANDLE out of the SM_HANDLE*/
    THANDLE(PTR(SM_HANDLE)) thandle = THANDLE_PTR_CREATE_WITH_MOVE(SM_HANDLE)(sm_handle, NULL); // This is a dummy value that is not used in the test.

    /*here be pretending usage*/

    /*get rid of it*/
    THANDLE_ASSIGN(PTR(SM_HANDLE))(&thandle, NULL);
}
