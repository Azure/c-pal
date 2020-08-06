// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CALL_ONCE_H
#define REAL_CALL_ONCE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CALL_ONCE_GLOBAL_MOCK_HOOK()        \
    MU_FOR_EACH_1(R2,                               \
        call_once_begin,                            \
        call_once_end                               \
    )

#ifdef __cplusplus
extern "C" {
#endif
    CALL_ONCE_RESULT real_call_once_begin(call_once_t* state);
    void real_call_once_end(call_once_t* state, bool success);

#ifdef __cplusplus
}
#endif

#endif //REAL_CALL_ONCE_H
