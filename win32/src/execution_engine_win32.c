// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include "windows.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/execution_engine.h"
#include "execution_engine_win32.h"
#include "azure_c_pal/refcount.h"

typedef struct EXECUTION_ENGINE_TAG
{
    PTP_POOL ptp_pool;
}EXECUTION_ENGINE;

DEFINE_REFCOUNT_TYPE(EXECUTION_ENGINE);

EXECUTION_ENGINE_HANDLE execution_engine_create(void* execution_engine_parameters)
{
    EXECUTION_ENGINE_HANDLE result;
    EXECUTION_ENGINE_PARAMETERS_WIN32 parameters_to_use;

    if (execution_engine_parameters == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_011: [ If execution_engine_parameters is NULL, execution_engine_create shall use the defaults DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
        parameters_to_use.min_thread_count = DEFAULT_MIN_THREAD_COUNT;
        parameters_to_use.max_thread_count = DEFAULT_MAX_THREAD_COUNT;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_002: [ execution_engine_parameters shall be interpreted as EXECUTION_ENGINE_PARAMETERS_WIN32. ]*/
        EXECUTION_ENGINE_PARAMETERS_WIN32* execution_engine_parameters_win32 = (EXECUTION_ENGINE_PARAMETERS_WIN32*)execution_engine_parameters;

        parameters_to_use.min_thread_count = execution_engine_parameters_win32->min_thread_count;
        parameters_to_use.max_thread_count = execution_engine_parameters_win32->max_thread_count;
    }

    PTP_POOL ptp_pool;

    /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_013: [ If max_thread_count is non-zero, but less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
    if ((parameters_to_use.max_thread_count != 0) &&
        (parameters_to_use.max_thread_count < parameters_to_use.min_thread_count))
    {
        LogError("Invalid arguments: execution_engine_parameters_win32->min_thread_count=%" PRIu32 ", execution_engine_parameters_win32->max_thread_count=%" PRIu32,
            parameters_to_use.min_thread_count, parameters_to_use.max_thread_count);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_003: [ execution_engine_create shall call CreateThreadpool to create the Win32 threadpool. ]*/
        ptp_pool = CreateThreadpool(NULL);
        if (ptp_pool == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
            LogLastError("CreateThreadpool failed");
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_004: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
            if (!SetThreadpoolThreadMinimum(ptp_pool, parameters_to_use.min_thread_count))
            {
                /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                LogLastError("SetThreadpoolThreadMinimum failed");
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_012: [ If max_thread_count is 0, execution_engine_create shall not set the maximum thread count. ]*/
                if (parameters_to_use.max_thread_count > 0)
                {
                    /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_005: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
                    SetThreadpoolThreadMaximum(ptp_pool, parameters_to_use.max_thread_count);
                }



                /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
                result = REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE);
                if (result == NULL)
                {
                    /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                    LogError("REFCOUNT_TYPE_CREATE failed.");
                }
                else
                {
                    /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
                    result->ptp_pool = ptp_pool;
                    goto all_ok;
                }
                
            }

            CloseThreadpool(ptp_pool);
        }
    }

    result = NULL;

all_ok:
    return result;
}

void execution_engine_dec_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_03_001: [ Otherwise execution_engine_dec_ref shall decrement the refcount.]*/
        if (DEC_REF(EXECUTION_ENGINE, execution_engine) == 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_WIN32_03_002: [ If the refcount is zero execution_engine_dec_ref shall close the threadpool. ]*/
            CloseThreadpool(execution_engine->ptp_pool);
            REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, execution_engine);
        }
    }
}

void execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_03_003: [ If execution_engine is NULL, execution_engine_inc_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_03_004: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine.]*/
        INC_REF(EXECUTION_ENGINE, execution_engine);
    }
}

PTP_POOL execution_engine_win32_get_threadpool(EXECUTION_ENGINE_HANDLE execution_engine)
{
    PTP_POOL result;

    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_009: [ If execution_engine is NULL, execution_engine_win32_get_threadpool shall fail and return NULL. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_WIN32_01_010: [ Otherwise, execution_engine_win32_get_threadpool shall return the threadpool handle created in execution_engine_create. ]*/
        result = execution_engine->ptp_pool;
    }

    return result;
}
