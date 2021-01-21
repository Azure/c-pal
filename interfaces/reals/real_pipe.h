// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_PIPE_H
#define REAL_PIPE_H

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_PIPE_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        pipe_popen, \
        pipe_pclose \
)

#ifdef __cplusplus
extern "C" {
#endif


FILE* real_pipe_popen(const char* command);

int real_pipe_pclose(FILE* stream, int* exit_code);

#ifdef __cplusplus
}
#endif

#endif // REAL_PIPE_H
