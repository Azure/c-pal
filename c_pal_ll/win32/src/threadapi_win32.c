// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/threadapi.h"

MU_DEFINE_ENUM_STRINGS(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    THREADAPI_RESULT result;
    if ((threadHandle == NULL) ||
        (func == NULL))
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(THREADAPI_RESULT, result));
    }
    else
    {
        *threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
        if(*threadHandle == NULL)
        {
            result = (GetLastError() == ERROR_OUTOFMEMORY) ? THREADAPI_NO_MEMORY : THREADAPI_ERROR;

            LogError("(result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(THREADAPI_RESULT, result));
        }
        else
        {
            result = THREADAPI_OK;
        }
    }

    return result;
}

THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int *res)
{
    THREADAPI_RESULT result = THREADAPI_OK;

    if (threadHandle == NULL)
    {
        result = THREADAPI_INVALID_ARG;
        LogError("(result = %" PRI_MU_ENUM ")", MU_ENUM_VALUE(THREADAPI_RESULT, result));
    }
    else
    {
        DWORD returnCode = WaitForSingleObject(threadHandle, INFINITE);

        if( returnCode != WAIT_OBJECT_0)
        {
            result = THREADAPI_ERROR;
            LogLastError("Error waiting for Single Object. Return Code: %lu. Error Code: %" PRI_MU_ENUM "", returnCode, MU_ENUM_VALUE(THREADAPI_RESULT, result));
        }
        else
        {
            if (res != NULL)
            {
                DWORD exit_code;
                if (!GetExitCodeThread(threadHandle, &exit_code)) //If thread end is signaled we need to get the Thread Exit Code;
                {
                    result = THREADAPI_ERROR;
                    LogError("Error Getting Exit Code. Error Code: %u.", (unsigned int)GetLastError());
                }
                else
                {
                    *res = (int)exit_code;
                }
            }
        }
        CloseHandle(threadHandle);
    }

    return result;
}

void ThreadAPI_Sleep(unsigned int milliseconds)
{
    Sleep(milliseconds);
}

uint32_t ThreadAPI_GetCurrentThreadId(void)
{
    return (uint32_t)GetCurrentThreadId();
}
