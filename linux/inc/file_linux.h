// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FILE_LINUX_H
#define FILE_LINUX_H

#include "file_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE_LINUX_IO_TAG
{
    void* handle;
    FILE_ASYNC_OPERATION type;
    FILE_IO_DATA data;
}FILE_LINUX_IO;

static void on_file_io_complete_linux( FILE_LINUX_IO* io);

#ifdef __cplusplus
}
#endif
#endif