// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "file.h"

typedef struct FILE_WRITE_DATA_CONTEXT_TAG
{
    FILE_WRITE_CB user_callback;
    void* user_context;
    const unsigned char* source;
    uint32_t size;
}FILE_WRITE_DATA_CONTEXT;

typedef struct FILE_READ_DATA_CONTEXT_TAG
{
    FILE_READ_CB user_callback;
    void* user_context;
    unsigned char* destination;
    uint32_t size;
}FILE_READ_DATA_CONTEXT;

#define FILE_ASYNC_OPERATION_VALUES \
    FILE_ASYNC_WRITE, \
    FILE_ASYNC_READ

MU_DEFINE_ENUM(FILE_ASYNC_OPERATION, FILE_ASYNC_OPERATION_VALUES)
MU_DEFINE_ENUM_STRINGS(FILE_ASYNC_OPERATION, FILE_ASYNC_OPERATION_VALUES)

typedef struct FILE_HANDLE_DATA_TAG
{
    PTP_POOL ptp_pool;
    HANDLE h_file;
    TP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP ptp_cleanup_group; /*the cleanup group of IO operations*/
    PTP_IO ptp_io;
    FILE_REPORT_FAULT user_report_fault_callback;
    void* user_report_fault_context;
    
}FILE_HANDLE_DATA;

typedef union FILE_IO_DATA_TAG
{
    FILE_READ_DATA_CONTEXT read_data;
    FILE_WRITE_DATA_CONTEXT write_data;
}FILE_IO_DATA;

typedef struct FILE_WIN32_IO_TAG
{
    OVERLAPPED ov;
    FILE_HANDLE handle;
    FILE_ASYNC_OPERATION type;
    FILE_IO_DATA data;
}FILE_WIN32_IO;

static void on_file_io_complete_win32(
    PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PVOID overlapped,
    ULONG io_result,
    ULONG_PTR number_of_bytes_transferred,
    PTP_IO io
);