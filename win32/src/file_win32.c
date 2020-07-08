// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "file.h"
#include "file_internal.h"

MU_DEFINE_ENUM_STRINGS(FILE_ASYNC_OPERATION, FILE_ASYNC_OPERATION_VALUES)

typedef struct FILE_HANDLE_DATA_TAG
{
    PTP_POOL ptpPool;
    HANDLE hFile;
    TP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP ptpcg; /*the cleanup group of IO operations*/
    PTP_IO ptpIo;
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
    HANDLE handle;
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