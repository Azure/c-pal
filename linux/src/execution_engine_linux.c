// Copyright (C) Microsoft Corporation. All rights reserved.

#include "c_pal/execution_engine.h"

EXECUTION_ENGINE_HANDLE execution_engine_create(void* execution_engine_parameters)
{
    (void)execution_engine_parameters;
    return NULL;
}

void execution_engine_dec_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    (void)execution_engine;
}

void execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    (void)execution_engine;
}
