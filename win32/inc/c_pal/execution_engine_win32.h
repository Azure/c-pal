// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_WIN32_H
#define EXECUTION_ENGINE_WIN32_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "windows.h"
#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, PTP_POOL, execution_engine_win32_get_threadpool, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_WIN32_H
