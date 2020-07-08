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

typedef struct FILE_LINUX_WRITE_TAG
{
    FILE_HANDLE_DATA handle;
    FILE_WRITE_DATA_CONTEXT write_data;
}FILE_LINUX_WRITE;

typedef struct FILE_LINUX_READ_TAG
{
    FILE_HANDLE_DATA handle;
    FILE_READ_DATA_CONTEXT read_data;
}FILE_LINUX_READ;

static void on_file_write_complete_linux( FILE_LINUX_WRITE* write_info);
static void on_file_read_complete_linux( FILE_LINUX_READ* read_info);