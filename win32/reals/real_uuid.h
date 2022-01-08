// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_UUID_H
#define REAL_UUID_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_ARITHMETIC_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        uuid_produce                                    \
    )

#ifdef __cplusplus
extern "C" {
#endif

    int real_uuid_produce(UUID_T destination);

#ifdef __cplusplus
}
#endif

#endif //REAL_UUID_H
