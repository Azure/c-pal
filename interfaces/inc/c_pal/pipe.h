// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PIPE_H
#define PIPE_H

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, FILE*, pipe_popen, const char*, command);

MOCKABLE_FUNCTION(, int, pipe_pclose, FILE*, stream, int*, exit_code);

#ifdef __cplusplus
}
#endif

#endif /* PIPE_H */
