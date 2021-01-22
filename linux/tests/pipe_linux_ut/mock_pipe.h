// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCK_PIPE_H
#define MOCK_PIPE_H

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, FILE*, mock_popen, char const*, _Command, char const*, _Mode);
MOCKABLE_FUNCTION(, int, mock_pclose, FILE*, _Stream);

#ifdef __cplusplus
}
#endif

#endif // MOCK_PIPE_H
