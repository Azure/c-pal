// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "file.h"
#include "file_internal.h"

MU_DEFINE_ENUM_STRINGS(FILE_ASYNC_OPERATION, FILE_ASYNC_OPERATION_VALUES)

typedef struct FILE_HANDLE_DATA_TAG
{
    int handle;
    FILE_REPORT_FAULT user_report_fault_callback;
    void* user_report_fault_context;
}FILE_HANDLE_DATA;

typedef struct FILE_LINUX_IO_TAG
{
    void* handle;
    FILE_ASYNC_OPERATION type;
    FILE_IO_DATA data;
}FILE_LINUX_IO;

static void on_file_io_complete_linux( FILE_LINUX_IO* io);