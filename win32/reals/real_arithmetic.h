// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_ARITHMETIC_H
#define REAL_ARITHMETIC_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_ARITHMETIC_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        umul64x64                                       \
    )

#ifdef __cplusplus
extern "C" {
#endif
    PAL_UINT128 real_umul64x64(uint64_t left, uint64_t right);

#ifdef __cplusplus
}
#endif

#endif //REAL_ARITHMETIC_H
