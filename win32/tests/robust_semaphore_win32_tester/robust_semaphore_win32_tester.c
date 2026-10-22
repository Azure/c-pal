// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* robust_semaphore_win32_int starts this process to hold a unit of a robust semaphore from another process, then kills it to check that the unit is given back */

#include <stdio.h>
#include <stdint.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"

#include "c_pal/robust_semaphore.h"

/* the test kills this process long before this */
#define TESTER_HOLD_MS 120000

static void on_acquire_complete(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    HANDLE held_event = context;
    (void)unit_index;

    if (result != ROBUST_SEMAPHORE_ACQUIRE_OK)
    {
        (void)printf("Acquiring a unit failed with %" PRI_MU_ENUM "\n", MU_ENUM_VALUE(ROBUST_SEMAPHORE_ACQUIRE_RESULT, result));
    }
    else if (!SetEvent(held_event))
    {
        (void)printf("SetEvent failed with %lu\n", GetLastError());
    }
    else
    {
        /* the test knows that this process holds the unit */
    }
}

/* holds a unit of a robust semaphore with 1 unit until it is killed by the test */
int main(int argc, char* argv[])
{
    int result;

    if (argc != 3)
    {
        (void)printf("Usage: robust_semaphore_win32_tester.exe <RobustSemaphoreName> <HeldEventName>\n");
        result = 1;
    }
    else if (gballoc_hl_init(NULL, NULL) != 0)
    {
        (void)printf("gballoc_hl_init failed\n");
        result = 2;
    }
    else
    {
        HANDLE held_event = OpenEventA(EVENT_MODIFY_STATE, FALSE, argv[2]);
        if (held_event == NULL)
        {
            (void)printf("OpenEventA(%s) failed with %lu\n", argv[2], GetLastError());
            result = 3;
        }
        else
        {
            ROBUST_SEMAPHORE_HANDLE robust_semaphore = robust_semaphore_create(argv[1], 1, NULL);
            if (robust_semaphore == NULL)
            {
                (void)printf("robust_semaphore_create(%s) failed\n", argv[1]);
                result = 4;
            }
            else
            {
                if (robust_semaphore_acquire_async(robust_semaphore, ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete, held_event) != 0)
                {
                    (void)printf("robust_semaphore_acquire_async failed\n");
                    result = 5;
                }
                else
                {
                    Sleep(TESTER_HOLD_MS);
                    result = 0;
                }
                robust_semaphore_destroy(robust_semaphore);
            }
            (void)CloseHandle(held_event);
        }
        gballoc_hl_deinit();
    }

    return result;
}
