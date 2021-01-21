// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "mock_pipe.h"

#undef popen
#define popen mock_popen
#undef pclose
#define pclose mock_pclose

#include "../../src/pipe_linux.c"
