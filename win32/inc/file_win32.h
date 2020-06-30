// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FILE_WIN32_H
#define FILE_WIN32_H

#include "windows.h"
#include "file_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
#endif