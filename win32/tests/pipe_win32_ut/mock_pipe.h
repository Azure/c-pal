// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCK_PIPE_H
#define MOCK_PIPE_H

#include <stdio.h>

#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, FILE*, mock_popen, char const*, _Command, char const*, _Mode);
MOCKABLE_FUNCTION(, int, mock_pclose, FILE*, _Stream);

#endif // MOCK_PIPE_H