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

typedef struct FILE_WIN32_IO_TAG
{
    OVERLAPPED ov;
    HANDLE handle;
    FILE_ASYNC_OPERATION type;
    FILE_IO_DATA data;
}FILE_WIN32_IO;

static VOID CALLBACK on_file_io_complete_win32( /*called when some read/write operation is finished*/
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_opt_ PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             NumberOfBytesTransferred,
    _Inout_     PTP_IO                Io
);
