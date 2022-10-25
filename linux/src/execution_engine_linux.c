// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

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

    result = REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE);
    if (result == NULL)
    {
        LogError("REFCOUNT_TYPE_CREATE failed.");
    }
    else
    {
        if (execution_engine_parameters == NULL)
        {
            result->params.min_thread_count = DEFAULT_MIN_THREAD_COUNT;
            result->params.max_thread_count = DEFAULT_MAX_THREAD_COUNT;
        }
        else
        {
            EXECUTION_ENGINE_PARAMETERS_LINUX* parameters_linux = (EXECUTION_ENGINE_PARAMETERS_LINUX*)execution_engine_parameters;

            result->params.min_thread_count = parameters_linux->min_thread_count;
            result->params.max_thread_count = parameters_linux->max_thread_count;
        }

        if ((result->params.max_thread_count != 0) &&
            (result->params.max_thread_count < result->params.min_thread_count))
        {
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
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        if (DEC_REF(EXECUTION_ENGINE, execution_engine) == 0)
        {
            REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, execution_engine);
        }
    }
}

void execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        INC_REF(EXECUTION_ENGINE, execution_engine);
    }
}

const EXECUTION_ENGINE_PARAMETERS_LINUX* execution_engine_linux_get_parameters(EXECUTION_ENGINE_HANDLE execution_engine)
{
    EXECUTION_ENGINE_PARAMETERS_LINUX* result;

    if (execution_engine == NULL)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        result = &execution_engine->params;
    }

    return result;
}
