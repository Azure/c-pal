// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <errno.h>
#include "mock_pipe.h"

#undef _popen
#define _popen mock_popen
#undef _pclose
#define _pclose mock_pclose

#include "../../src/pipe_win32.c"
