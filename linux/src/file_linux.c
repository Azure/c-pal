// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "c_pal/file.h"

typedef struct FILE_HANDLE_DATA_TAG
{
    int handle;
    FILE_REPORT_FAULT user_report_fault_callback;
    void* user_report_fault_context;
}FILE_HANDLE_DATA;

typedef struct FILE_LINUX_WRITE_TAG
{
    FILE_HANDLE handle;
    FILE_CB user_callback;
    void* user_context;
    const unsigned char* source;
    uint32_t size;
}FILE_LINUX_WRITE;

typedef struct FILE_LINUX_READ_TAG
{
    FILE_HANDLE handle;
    FILE_CB user_callback;
    void* user_context;
    unsigned char* destination;
    uint32_t size;
}FILE_LINUX_READ;

static void on_file_write_complete_linux( FILE_LINUX_WRITE* write_info);
static void on_file_read_complete_linux( FILE_LINUX_READ* read_info);
