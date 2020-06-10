// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EXECUTION_ENGINE_TAG* EXECUTION_ENGINE_HANDLE;

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_H
