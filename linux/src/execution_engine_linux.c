// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_logging/logger.h"

#include "c_pal/execution_engine_linux.h"
#include "c_pal/refcount.h"
#include "c_pal/execution_engine.h"

typedef struct EXECUTION_ENGINE_TAG
{
    EXECUTION_ENGINE_PARAMETERS_LINUX params;
}EXECUTION_ENGINE;

DEFINE_REFCOUNT_TYPE(EXECUTION_ENGINE);

EXECUTION_ENGINE_HANDLE execution_engine_create(void* execution_engine_parameters)
{
    EXECUTION_ENGINE_HANDLE result;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
    result = REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE);
    if (result == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
        LogError("REFCOUNT_TYPE_CREATE failed.");
    }
    else
    {
        if (execution_engine_parameters == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
            result->params.min_thread_count = DEFAULT_MIN_THREAD_COUNT;
            result->params.max_thread_count = DEFAULT_MAX_THREAD_COUNT;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
            EXECUTION_ENGINE_PARAMETERS_LINUX* parameters_linux = (EXECUTION_ENGINE_PARAMETERS_LINUX*)execution_engine_parameters;

            result->params.min_thread_count = parameters_linux->min_thread_count;
            result->params.max_thread_count = parameters_linux->max_thread_count;
        }

        if ((result->params.max_thread_count != 0) &&
            (result->params.max_thread_count < result->params.min_thread_count))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
            LogError("Invalid arguments: execution_engine_parameters_win32->min_thread_count=%" PRIu32 ", execution_engine_parameters_win32->max_thread_count=%" PRIu32,
                result->params.min_thread_count, result->params.max_thread_count);
        }
        else
        {
            LogInfo("Creating execution engine with min thread count=%" PRIu32 ", max thread count=%" PRIu32 "", result->params.min_thread_count, result->params.max_thread_count);

            goto all_ok;
        }
        REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, result);
        result = NULL;
    }
all_ok:
    return result;
}

void execution_engine_dec_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
        if (DEC_REF(EXECUTION_ENGINE, execution_engine) == 0)
        {

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_009: [ If the refcount is zero execution_engine_dec_ref shall free the memory for EXECUTION_ENGINE. ]*/
            REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, execution_engine);
        }
    }
}

void execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_010: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_011: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
        INC_REF(EXECUTION_ENGINE, execution_engine);
    }
}

const EXECUTION_ENGINE_PARAMETERS_LINUX* execution_engine_linux_get_parameters(EXECUTION_ENGINE_HANDLE execution_engine)
{
    EXECUTION_ENGINE_PARAMETERS_LINUX* result;

    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_012: [ If execution_engine is NULL, execution_engine_linux_get_parameters shall fail and return NULL. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_013: [ Otherwise, execution_engine_linux_get_parameters shall return the parameters in EXECUTION_ENGINE. ]*/
        result = &execution_engine->params;
    }

    return result;
}
