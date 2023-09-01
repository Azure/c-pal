// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_LINUX_H
#define EXECUTION_ENGINE_LINUX_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_LINUX_H
