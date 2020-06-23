// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#undef syscall 
#define syscall mock_syscall

#include "../../src/sync_linux.c"
