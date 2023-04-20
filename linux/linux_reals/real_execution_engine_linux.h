// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_EXECUTION_ENGINE_LINUX_H
#define REAL_EXECUTION_ENGINE_LINUX_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_EXECUTION_ENGINE_LINUX_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        execution_engine_linux_get_parameters \
    )

#ifdef __cplusplus
extern "C" {
#endif

    const EXECUTION_ENGINE_PARAMETERS_LINUX* real_execution_engine_linux_get_parameters(EXECUTION_ENGINE_HANDLE execution_engine);

#ifdef __cplusplus
}
#endif

#endif // REAL_EXECUTION_ENGINE_LINUX_H
