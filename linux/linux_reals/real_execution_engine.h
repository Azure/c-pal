// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_EXECUTION_ENGINE_H
#define REAL_EXECUTION_ENGINE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_EXECUTION_ENGINE_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        execution_engine_create,                                       \
        execution_engine_dec_ref, \
        execution_engine_inc_ref \
    )

#ifdef __cplusplus
extern "C" {
#endif

    EXECUTION_ENGINE_HANDLE real_execution_engine_create(void* execution_engine_parameters);
    void real_execution_engine_dec_ref(EXECUTION_ENGINE_HANDLE execution_engine);
    void real_execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine);

#ifdef __cplusplus
}
#endif

#endif // REAL_EXECUTION_ENGINE_H
