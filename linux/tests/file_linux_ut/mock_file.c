// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>

#include "mock_file.h"
#define open mock_open
#define close mock_close
#define aio_write mock_aio_write
#define aio_read mock_aio_read
#define aio_return mock_aio_return
#include "../../src/file_linux.c"
