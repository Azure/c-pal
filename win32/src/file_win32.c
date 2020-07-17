// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "windows.h"

#include "azure_c_logging/xlogging.h"
#include "execution_engine_win32.h"
#include "azure_c_pal/gballoc.h"
#include "azure_c_pal/file.h"

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

typedef struct FILE_WIN32_IO_TAG
{
    OVERLAPPED ov;
    FILE_HANDLE handle;
    FILE_CB user_callback;
    void* user_context;
    uint32_t size;
}FILE_WIN32_IO;

static void on_file_io_complete_win32(
    PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PVOID overlapped,
    ULONG io_result,
    ULONG_PTR number_of_bytes_transferred,
    PTP_IO io
);

static VOID NTAPI on_close_threadpool_group_member(
     PVOID object_context, /*what was passed @  CreateThreadpoolIo*/
     PVOID cleanup_context /*what was passed @  CloseThreadpoolCleanupGroupMembers*/
)
{
    (void)object_context;
    (void)cleanup_context;
}

IMPLEMENT_MOCKABLE_FUNCTION(
    ,
    FILE_HANDLE, file_create,
    EXECUTION_ENGINE_HANDLE, execution_engine,
    const char*, full_file_name,
    FILE_REPORT_FAULT, user_report_fault_callback,
    void*, user_report_fault_context
)
{
    FILE_HANDLE handle = (FILE_HANDLE)NULL;
    bool result = true;
    if
    (
        /*Codes_SRS_FILE_43_033: [ If execution_engine is NULL, file_create shall fail and return NULL. ]*/
        /*Codes_SRS_FILE_WIN32_43_040: [ If execution_engine is NULL, file_create shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /*Codes_SRS_FILE_43_002: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
        /*Codes_SRS_FILE_WIN32_43_048: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
        (full_file_name == NULL) ||
        /*Codes_SRS_FILE_43_037: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
        /*Codes_SRS_FILE_WIN32_43_059: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
        (strlen(full_file_name) == 0)
    )
    {
        LogError("Invalid arguments to file_create: EXECUTION_ENGINE_HANDLE execution_engine=%p, const char* full_file_name=%s, FILE_REPORT_FAULT user_report_callback=%p, void* user_report_faul_context=%p",
            execution_engine, full_file_name, user_report_fault_callback, user_report_fault_context);
        result = false;
    }
    else
    {
        /*Codes_SRS_FILE_WIN32_43_041: [ file_create shall allocate a FILE_HANDLE. ]*/
        handle = malloc(sizeof(FILE_HANDLE_DATA));
        if (handle == NULL) {
            LogError("Failure in malloc");
            result = false;
        }
        else
        {
            /*Codes_SRS_FILE_43_003: [ If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
            /*Codes_SRS_FILE_43_001: [ file_create shall open the file named full_file_name for asynchronous operations and return its handle. ]*/
            /*Codes_SRS_FILE_WIN32_43_001: [ file_create shall call CreateFileA with full_file_name as lpFileName, GENERIC_READ|GENERIC_WRITE as dwDesiredAccess, FILE_SHARED_READ as dwShareMode, NULL as lpSecurityAttributes, OPEN_ALWAYS as dwCreationDisposition, FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH as dwFlagsAndAttributes and NULL as hTemplateFile. ]*/
            if ((handle->h_file = CreateFileA(
                full_file_name,                                       /*_In_     LPCTSTR               lpFileName*/
                GENERIC_READ | GENERIC_WRITE,                       /*_In_     DWORD                 dwDesiredAccess*/
                FILE_SHARE_READ,                                    /*_In_     DWORD                 dwShareMode*/
                NULL,                                               /*_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes*/
                OPEN_ALWAYS,                                        /*_In_     DWORD                 dwCreationDisposition*/
                FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,     /*_In_     DWORD                 dwFlagsAndAttributes*/
                NULL                                                /*_In_opt_ HANDLE                hTemplateFile*/
            )) == INVALID_HANDLE_VALUE)
            {
                LogError("Failure in CreateFileA, full_file_name=%s", full_file_name);
                result = false;
            }
            else
            {
                /*Codes_SRS_FILE_WIN32_43_002: [ file_create shall call SetFileCompletionNotificationModes to disable calling the completion port when an async operations finishes synchrounously.]*/
                if (!SetFileCompletionNotificationModes(handle->h_file, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS))
                {
                    LogError("Failure in SetFileCompletionNotificationModes, full_file_name=%s", full_file_name);
                    result = false;
                }
                else
                {
                    /*Codes_SRS_FILE_WIN32_43_003: [ file_create shall initialize a threadpool environment by calling InitializeThreadpolEnvironment.]*/
                    InitializeThreadpoolEnvironment(&handle->cbe);

                    /*Codes_SRS_FILE_WIN32_43_004: [ file_create shall obtain a PTP_POOL struct by calling execution_engine_win32_get_threadpool on execution_engine.]*/
                    handle->ptp_pool = execution_engine_win32_get_threadpool(execution_engine);

                    /*Codes_SRS_FILE_WIN32_43_005: [ file_create shall register the threadpool environment by calling SetThreadpoolCallbackPool on the initialized threadpool environment and the obtained ptp_pool ]*/
                    SetThreadpoolCallbackPool(&handle->cbe, handle->ptp_pool);

                    /*Codes_SRS_FILE_WIN32_43_006: [ file_create shall create a cleanup group by calling CreateThreadpoolCleanupGroup.]*/
                    handle->ptp_cleanup_group = CreateThreadpoolCleanupGroup();

                    if (handle->ptp_cleanup_group == NULL)
                    {
                        LogError("Failure in CreateThreadpoolCleanupGroup, full_file_name=%s", full_file_name);
                        result = false;
                    }
                    else
                    {
                        /*Codes_SRS_FILE_WIN32_43_007: [ file_create shall register the cleanup group with the threadpool environment by calling SetThreadpoolCallbackCleanupGroup.]*/
                        SetThreadpoolCallbackCleanupGroup(&handle->cbe, handle->ptp_cleanup_group, on_close_threadpool_group_member);

                        /*Codes_SRS_FILE_WIN32_43_033: [ file_create shall create a threadpool io with the allocated FILE_HANDLE and on_file_io_complete_win32 as a callback by calling CreateThreadpoolIo]*/
                        handle->ptp_io = CreateThreadpoolIo(handle->h_file, on_file_io_complete_win32, NULL, &handle->cbe);

                        if (handle->ptp_io == NULL)
                        {
                            LogError("Failure in CreateThreadpoolIo, full_file_name=%s", full_file_name);
                            result = false;
                        }
                        if (!result)
                        {
                            CloseThreadpoolCleanupGroup(handle->ptp_cleanup_group);
                        }
                    }
                    if (!result)
                    {
                        DestroyThreadpoolEnvironment(&handle->cbe);
                    }
                }
            }
            /*Codes_SRS_FILE_43_034: [ If there are any failures, file_create shall fail and return NULL. ]*/
            if (!result)
            {
                free(handle);
            }
        }
    }
    /*Codes_SRS_FILE_WIN32_43_008: [ If there are any failures, file_create shall return NULL.]*/
    if (!result)
    {
        return NULL;
    }

    /*Codes_SRS_FILE_43_038: [ file_create shall register user_report_fault_callback with argument user_report_fault_context as the callback function to be called when the callback specified by the user for a specific asynchronous operation cannot be called. ]*/
    handle->user_report_fault_callback = user_report_fault_callback;
    handle->user_report_fault_context = user_report_fault_context;
    /*Codes_SRS_FILE_WIN32_43_009: [ file_create shall succeed and return a non - NULL value.]*/
    return handle;
}


IMPLEMENT_MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle)
{
    /*Codes_SRS_FILE_43_005: [ If handle is NULL, file_destroy shall return. ]*/
    /*Codes_SRS_FILE_WIN32_43_049: [ If handle is NULL, file_destroy shall return.]*/
    if (handle != NULL)
    {
        /*Codes_SRS_FILE_43_006: [ file_destroy shall wait for all pending I/O operations to complete. ]*/
        /*Codes_SRS_FILE_WIN32_43_011: [ file_destroy shall wait for all I/O to complete by calling WaitForThreadpoolIoCallbacks. ]*/
        WaitForThreadpoolIoCallbacks(handle->ptp_io, FALSE);
        /*Codes_SRS_FILE_WIN32_43_012: [ file_destroy shall close the cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
        CloseThreadpoolCleanupGroup(handle->ptp_cleanup_group);
        /*Codes_SRS_FILE_WIN32_43_013: [ file_destroy shall destroy the environment by calling DestroyThreadpoolEnvironment. ]*/
        DestroyThreadpoolEnvironment(&(handle->cbe));
        /*Codes_SRS_FILE_43_007: [ file_destroy shall close the file handle handle. ]*/
        /*Codes_SRS_FILE_WIN32_43_016: [ file_destroy shall call CloseHandle on handle->h_file ]*/
        if (!CloseHandle(handle->h_file))
        {
            LogError("failure in CloseHandle");
        }
        /*Codes_SRS_FILE_WIN32_43_015: [ file_destroy shall close the threadpool IO by calling CloseThreadPoolIo. ]*/
        CloseThreadpoolIo(handle->ptp_io);
        /*Codes_SRS_FILE_WIN32_43_042: [ file_destroy shall free the handle.]*/
        free(handle);
    }
    else
    {
        LogError("Invalid argument to file_destroy: FILE_HANDLE=%p", handle);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)
{
    FILE_WRITE_ASYNC_RESULT result;
    if
    (
        /*Codes_SRS_FILE_43_009: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_043: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS.]*/
        (handle == NULL) ||
        /*Codes_SRS_FILE_43_010: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_044: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS.]*/
        (source == NULL) ||
        /*Codes_SRS_FILE_43_012: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_045: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS.]*/
        (user_callback == NULL)||
        /*Codes_SRS_FILE_43_040: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_060: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        ((position + size) > INT64_MAX)||
        /*Codes_SRS_FILE_43_042: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_061: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        (size == 0)
    )
    {
        LogError("Invalid arguments to file_write_async: FILE_HANDLE file_handle=%p, const unsigned char* source=%p, uin32_t size=%" PRIu32 ", uint64_t position=%" PRIu64 ", FILE_CB user_callback=%p, void* user_context=%p",
            handle, source, size, position, user_callback, user_context);
        result = FILE_WRITE_ASYNC_INVALID_ARGS;
    }
    else
    {
        bool callback_will_be_called = false;
        /*Codes_SRS_FILE_WIN32_43_018: [ file_write_async shall allocate a context to store the allocated OVERLAPPED struct, handle, size, user_callback and user_context. ]*/
        FILE_WIN32_IO* io = malloc(sizeof(FILE_WIN32_IO));
        if (io == NULL)
        {
            /*Codes_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
            /*Codes_SRS_FILE_WIN32_43_057: [ If there are any other failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR.]*/
            LogError("failure in malloc");
            result = FILE_WRITE_ASYNC_ERROR;
        }
        else
        {
            io->handle = handle;
            /*Codes_SRS_FILE_WIN32_43_020: [ file_write_async shall allocate an OVERLAPPED struct and populate it with the created event and position. ]*/
            (void)memset(&(io->ov), 0, sizeof(OVERLAPPED));
            /*Codes_SRS_FILE_WIN32_43_054: [ file_write_async shall create an event by calling CreateEvent.]*/
            io->ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (io->ov.hEvent == NULL)
            {
                /*Codes_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
                /*Codes_SRS_FILE_WIN32_43_057: [ If there are any other failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR.]*/
                LogError("failure in CreateEvent");
                result = FILE_WRITE_ASYNC_ERROR;
            }
            else
            {
                io->ov.Offset = position & 0xFFFFFFFFULL;
                io->ov.OffsetHigh = position >> 32;

                io->user_callback = user_callback;
                io->user_context = user_context;

                io->size = size;

                /*Codes_SRS_FILE_WIN32_43_017: [ file_write_async shall call StartThreadpoolIo.]*/
                StartThreadpoolIo(handle->ptp_io);

                /*Codes_SRS_FILE_43_014: [ file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
                /*Codes_SRS_FILE_43_041: [ If position + size is greater than the size of the file and the call to write is succesfull, file_write_async shall grow the file to accomodate the write. ]*/
                /*Codes_SRS_FILE_WIN32_43_021: [ file_write_async shall call WriteFile with handle, source, size and the allocated OVERLAPPED struct.]*/
                if (WriteFile(handle->h_file, source, size, NULL, &(io->ov)) == FALSE)
                {
                    if (GetLastError() == ERROR_IO_PENDING)
                    {
                        /*Codes_SRS_FILE_43_008: [ file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
                        /*Codes_SRS_FILE_43_030: [ file_write_async shall succeed and return FILE_WRITE_ASYNC_OK. ]*/
                        /*Codes_SRS_FILE_WIN32_43_022: [ If WriteFile fails synchronously and GetLastError indicates ERROR_IO_PENDING then file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
                        callback_will_be_called = true;
                        result = FILE_WRITE_ASYNC_OK;
                    }
                    else
                    {
                        /*Codes_SRS_FILE_43_035: [ If the call to write the file fails, file_write_async shall fail and return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
                        /*Codes_SRS_FILE_WIN32_43_023: [ If WriteFile fails synchronously and GetLastError does not indicate ERROR_IO_PENDING then file_write_async shall fail, call CancelThreadpoolIo and return FILE_WRITE_ASYNC_WRITE_ERROR.]*/
                        LogError("failure in WriteFile");
                        CancelThreadpoolIo(handle->ptp_io);
                        result = FILE_WRITE_ASYNC_WRITE_ERROR;
                    }
                }
                else
                {
                    /*Codes_SRS_FILE_43_008: [ file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
                    /*Codes_SRS_FILE_43_030: [ file_write_async shall succeed and return FILE_WRITE_ASYNC_OK. ]*/
                    /*Codes_SRS_FILE_WIN32_43_024: [ If WriteFile succeeds synchronously then file_write_async shall succeed, call CancelThreadpoolIo, call user_callback and return FILE_WRITE_ASYNC_OK.]*/
                    CancelThreadpoolIo(handle->ptp_io);
                    user_callback(user_context, true);
                    result = FILE_WRITE_ASYNC_OK;
                }
                if (!callback_will_be_called)
                {
                    if (!CloseHandle(io->ov.hEvent))
                    {
                        LogError("Failure in CloseHandle");
                    }
                }
            }
            if (!callback_will_be_called)
            {
                free(io);
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, unsigned char*, destination, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)
{
    FILE_READ_ASYNC_RESULT result;
    if
    (
        /*Codes_SRS_FILE_43_017: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_046: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (handle == NULL) ||
        /*Codes_SRS_FILE_43_032: [ If destination is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_051: [ If destination is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (destination == NULL) ||
        /*Codes_SRS_FILE_43_020: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_047: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (user_callback == NULL) ||
        /*Codes_SRS_FILE_43_043: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_WIN32_43_062: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (size == 0)
    )
    {
        LogError("Invalid arguments to file_read_async: FILE_HANDLE file_handle=%p, unsigned char* destination=%p, uin32_t size=%" PRIu32 ", uint64_t position=%" PRIu64 ", FILE_CB user_callback=%p, void* user_context=%p",
            handle, destination, size, position, user_callback, user_context);
        result = FILE_READ_ASYNC_INVALID_ARGS;
    }
    else
    {
        bool callback_will_be_called = false;
        /*Codes_SRS_FILE_WIN32_43_026: [ file_read_async shall allocate a context to store the allocated OVERLAPPED struct, destination, handle, size, user_callback and user_context ]*/
        FILE_WIN32_IO* io = malloc(sizeof(FILE_WIN32_IO));
        if (io == NULL)
        {
            /*Codes_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
            /*Codes_SRS_FILE_WIN32_43_058: [ If there are any other failures, file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
            LogError("Failure in malloc");
            result = FILE_READ_ASYNC_ERROR;
        }
        else
        {
            io->handle = handle;
            /*Codes_SRS_FILE_WIN32_43_028: [ file_read_async shall allocate an OVERLAPPED struct and populate it with the created event and position. ]*/
            (void)memset(&(io->ov), 0, sizeof(OVERLAPPED));
            /*Codes_SRS_FILE_WIN32_43_055: [ file_read_async shall create an event by calling CreateEvent. ]*/
            io->ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (io->ov.hEvent == NULL)
            {
                /*Codes_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
                /*Codes_SRS_FILE_WIN32_43_058: [ If there are any other failures, file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
                LogError("Failure in CreateEvent");
                result = FILE_READ_ASYNC_ERROR;
            }
            else
            {
                io->ov.Offset = position & 0xFFFFFFFFULL;
                io->ov.OffsetHigh = position >> 32;

                io->user_callback = user_callback;
                io->user_context = user_context;

                io->size = size;

                /*Codes_SRS_FILE_43_021: [ file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
                /*Codes_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
                /*Codes_SRS_FILE_WIN32_43_025: [ file_read_async shall call StartThreadpoolIo. ]*/
                StartThreadpoolIo(handle->ptp_io);
                /*Codes_SRS_FILE_WIN32_43_029: [ file_read_async shall call ReadFile with handle, destination, size and the allocated OVERLAPPED struct.]*/
                if (ReadFile(handle->h_file, destination, size, NULL, &(io->ov)) == FALSE)
                {
                    if (GetLastError() == ERROR_IO_PENDING)
                    {
                        /*Codes_SRS_FILE_43_016: [ file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
                        /*Codes_SRS_FILE_43_031: [ file_read_async shall succeed and return FILE_READ_ASYNC_OK. ]*/
                        /*Codes_SRS_FILE_WIN32_43_030: [ If ReadFile fails synchronously and GetLastError indicates ERROR_IO_PENDING then file_read_async shall succeed and return FILE_READ_ASYNC_OK. ]*/
                        callback_will_be_called = true;
                        result = FILE_READ_ASYNC_OK;
                    }
                    else
                    {
                        /*Codes_SRS_FILE_43_036: [ If the call to read the file fails, file_read_async shall fail and return FILE_READ_ASYNC_READ_ERROR. ]*/
                        /*Codes_SRS_FILE_WIN32_43_031: [ If ReadFile fails synchronously and GetLastError does not indicate ERROR_IO_PENDING then file_read_async shall fail, call CancelThreadpoolIo and return FILE_READ_ASYNC_WRITE_ERROR. ]*/
                        LogError("Failure in ReadFile");
                        CancelThreadpoolIo(handle->ptp_io);
                        result = FILE_READ_ASYNC_READ_ERROR;
                    }
                }
                else
                {
                    /*Codes_SRS_FILE_43_016: [ file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
                    /*Codes_SRS_FILE_43_031: [ file_read_async shall succeed and return FILE_READ_ASYNC_OK. ]*/
                    /*Codes_SRS_FILE_WIN32_43_032: [ If ReadFile succeeds synchronously then file_read_async shall succeed, call CancelThreadpoolIo, call user_callback and return FILE_READ_ASYNC_OK. ]*/
                    CancelThreadpoolIo(handle->ptp_io);
                    user_callback(user_context, true);
                    result = FILE_READ_ASYNC_OK;
                }
                if (!callback_will_be_called)
                {
                    if (!CloseHandle(io->ov.hEvent))
                    {
                        LogError("Failure in CloseHandle");
                    }
                }
            }
            if (!callback_will_be_called)
            {
                free(io);
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)
{
    (void)handle;
    (void)desired_size;
    /*Codes_SRS_FILE_WIN32_43_050: [ file_extend shall return 0. ]*/
    return 0;
}

static void on_file_io_complete_win32(
    PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PVOID overlapped,
    ULONG io_result,
    ULONG_PTR number_of_bytes_transferred,
    PTP_IO io
)
{
    (void)instance;
    (void)context;
    (void)io_result;
    (void)number_of_bytes_transferred;
    (void)io;

    void (*callback_function)();
    void* callback_context;
    HANDLE handle = NULL;
    bool report_fault = false;

    /*Codes_SRS_FILE_WIN32_43_034: [ on_file_io_complete_win32 shall recover the file handle, the number of bytes requested by the user, user_callback and user_context from the context containing overlapped. ]*/
    FILE_WIN32_IO* io_context = CONTAINING_RECORD(overlapped, FILE_WIN32_IO, ov);
    if (io_context->user_callback == NULL)
    {
        LogError("Could not recover user-specified callback");
        report_fault = true;
        callback_function = (FILE_REPORT_FAULT)(io_context->handle->user_report_fault_callback);
        callback_context = io_context->handle->user_report_fault_context;
    }
    else
    {
        callback_function = (FILE_CB)(io_context->user_callback);
        callback_context = io_context->user_context;
        handle = io_context->handle->h_file;
    }

    DWORD num_bytes;
    /*Codes_SRS_FILE_WIN32_43_063: [ on_file_io_complete_win32 shall call GetOverlappedResult to determine if the asynchronous operation was successful. ]*/
    bool overlapped_result = GetOverlappedResult(handle, overlapped, &num_bytes, FALSE);
    (void)num_bytes;
   
    /*Codes_SRS_FILE_WIN32_43_067: [ on_file_io_complete_win32 shall compare number_of_bytes_transferred to the number of bytes requested by the user. ]*/
    bool all_bytes_were_transferred = (uint32_t)number_of_bytes_transferred == io_context->size;

    CloseHandle(io_context->ov.hEvent);
    free(io_context);

    if (!overlapped_result)
    {
        DWORD error = GetLastError();
        LogError("Error in asynchronous operation. Error code =%" PRIu32 "", error);
    }
    if (!all_bytes_were_transferred)
    {
        LogError("All bytes were not transferred.");
    }


    if (report_fault)
    {
        if(callback_function != NULL)
        {
            ((FILE_REPORT_FAULT)callback_function)(callback_context, "Could not determine recover user_callback.");
        }
    }
    else
    {
        /*Codes_SRS_FILE_WIN32_43_066: [ on_file_io_complete_win32 shall call user_callback with is_successful as true if and only if GetOverlappedResult returns true and number_of_bytes_transferred is equal to the number of bytes requested by the user. ]*/
        /*Codes_SRS_FILE_WIN32_43_068: [ If either GetOverlappedResult returns false or number_of_bytes_transferred is not equal to the bytes requested by the user, on_file_io_complete_win32 shall return false. ]*/
        ((FILE_CB)callback_function)(callback_context, overlapped_result && all_bytes_were_transferred);
    }
}
