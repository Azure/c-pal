// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <errno.h> // IWYU pragma: keep

#undef syscall 
#define syscall mock_syscall
#undef errno
#define errno mock_errno

extern int mock_errno;

#include "../../src/sync_linux.c"
