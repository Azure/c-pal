// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_LAZY_INIT_H
#define REAL_LAZY_INIT_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK()       \
    MU_FOR_EACH_1(R2,                               \
        lazy_init                                   \
    )

#ifdef __cplusplus
extern "C" {
#endif

    LAZY_INIT_RESULT real_lazy_init(call_once_t* lazy, LAZY_INIT_FUNCTION do_init, void* init_params);

#ifdef __cplusplus
}
#endif

#endif //REAL_LAZY_INIT_H
