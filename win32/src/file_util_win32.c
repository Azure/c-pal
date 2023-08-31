// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep

#include "c_logging/xlogging.h"
#include "c_pal/interlocked.h"

#include "c_pal/file_util.h"

typedef struct FILE_WIN_TAG
{
    HANDLE file_handle;
    TP_CALLBACK_ENVIRON cbe;
    PTP_POOL ptpp;
    THANDLE(THREADPOOL) threadpool;
    PTP_CLEANUP_GROUP ptpcg;
    PTP_IO ptpio;

} FILE_WIN;


// void file_util_initialize_threadpool_environment(PTP_CALLBACK_ENVIRON pcbe)
// {

//     InitializeThreadpoolEnvironment(pcbe);
// }

// void file_util_set_threadpool_callback_pool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp)
// {
//     SetThreadpoolCallbackPool(pcbe, ptpp);
// }

// PTP_CLEANUP_GROUP file_util_create_threadpool_cleanup_group()
// {
//     return CreateThreadpoolCleanupGroup();
// }

static VOID NTAPI onCloseThreadpoolCleanupGroupMember(
    _Inout_opt_ PVOID ObjectContext, /*what was passed @  CreateThreadpoolIo*/
    _Inout_opt_ PVOID CleanupContext /*what was passed @  CloseThreadpoolCleanupGroupMembers*/
)
{
    (void)ObjectContext;
    (void)CleanupContext;
}

HANDLE file_util_open_file(const char* full_file_name, uint32_t access, uint32_t share_mode, LPSECURITY_ATTRIBUTES security_attributes, 
                    uint32_t creation_disposition, uint32_t flags_and_attributes, HANDLE template_file)
{
    FILE_WIN* new_file;
    if(full_file_name == NULL || full_file_name[0] == '\0')
    {
        LogError("Invalid arguments to file_util_open_file: full_file_name = %s, uint32_t access = %u, uint32_t share_mode = %u, LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %u, uint32_t flags_and_attributes = %u, HANDLE template_file = %p",
                    full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
        new_file = INVALID_HANDLE_VALUE;
        return new_file;
    }
    else
    {
        new_file = malloc(sizeof(FILE_WIN));
        if(new_file == NULL)
        {
            LogError("Failure in malloc");
            new_file = INVALID_HANDLE_VALUE;
        }
        else
        {
            new_file->file_handle = CreateFileA(full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
            if (new_file->file_handle == INVALID_HANDLE_VALUE)
            {
                DWORD last_error = GetLastError();
                (void)last_error;
                LogError("unable to create file handle");
            }
            new_file->ptpp = CreateThreadpool(NULL);
            new_file->ptpcg = CreateThreadpoolCleanupGroup();
        }
        return new_file;
    }
}

bool file_util_close_file(HANDLE handle_input)
{
    FILE_WIN* new_file = (FILE_WIN*)handle_input;
    bool success = CloseHandle(new_file->file_handle);
    free(new_file);
    return success;
}

bool file_util_write_file(HANDLE handle_input, LPCVOID buffer, uint32_t number_of_bytes_to_write, LPOVERLAPPED overlapped)
{
    FILE_WIN* new_file = (FILE_WIN*)handle_input;
    return WriteFile(new_file->file_handle, buffer, number_of_bytes_to_write, NULL, overlapped);

}

bool file_util_delete_file(LPCSTR full_file_name)
{
    return DeleteFileA(full_file_name);
}

PTP_IO file_util_create_threadpool_io(HANDLE handle_input, PTP_WIN32_IO_CALLBACK callback_function, PVOID pv)
{
    if(handle_input == NULL || callback_function == NULL)
    {
        LogError("Invalid inputs to file_util_create_threadpool: HANDLE handle_input = %p, PTP_WIN32_IO_CALLBACK = %p",
                    handle_input, callback_function);
        return NULL;
    }
    else
    {
        FILE_WIN* new_file = (FILE_WIN*)handle_input;
    
        InitializeThreadpoolEnvironment(&new_file->cbe);
        SetThreadpoolCallbackPool(&new_file->cbe, new_file->ptpp);
        SetThreadpoolCallbackCleanupGroup(&new_file->cbe, new_file->ptpcg, onCloseThreadpoolCleanupGroupMember);
        new_file->ptpio = CreateThreadpoolIo(new_file->file_handle, callback_function, pv, &new_file->cbe);
        StartThreadpoolIo(new_file->ptpio);
        return new_file->ptpio;
    }
}

PTP_CLEANUP_GROUP file_util_create_threadpool_cleanup_group()
{
    return CreateThreadpoolCleanupGroup();
}

bool file_util_set_file_completion_notification_modes(HANDLE handle_in, UCHAR flags)
{
    FILE_WIN* new_file = (FILE_WIN*)handle_in;
    return SetFileCompletionNotificationModes(new_file->file_handle, flags);
}

HANDLE file_util_create_event(LPSECURITY_ATTRIBUTES lpEventAttributes, bool bManualReset, bool bInitialState, LPCSTR lpName)
{
    return CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
}

bool file_util_query_performance_counter(LARGE_INTEGER* performance_count)
{
    return QueryPerformanceCounter(performance_count);
}

void file_util_cancel_threadpool_io(PTP_IO pio)
{
    CancelThreadpoolIo(pio);
}

bool file_util_read_file(HANDLE handle_in, LPVOID buffer, DWORD number_of_bytes_to_read, LPDWORD number_of_bytes_read, LPOVERLAPPED overlapped)
{
    FILE_WIN* new_file = (FILE_WIN*)handle_in;
    return ReadFile(new_file->file_handle, buffer, number_of_bytes_to_read, number_of_bytes_read, overlapped);
}

bool file_util_set_file_information_by_handle(HANDLE handle_in, FILE_INFO_BY_HANDLE_CLASS file_info_class, LPVOID file_info, DWORD buffer_size)
{
    return SetFileInformationByHandle(handle_in, file_info_class, file_info, buffer_size);
}

void file_util_close_threadpool_cleanup_group(PTP_CLEANUP_GROUP ptpcg)
{
    CloseThreadpoolCleanupGroup(ptpcg);
}

void file_util_destroy_threadpool_environment(PTP_CALLBACK_ENVIRON pcbe)
{
    DestroyThreadpoolEnvironment(pcbe);
}

void file_util_close_threadpool_io(PTP_IO pio)
{
    CloseThreadpoolIo(pio);
}

bool file_util_get_file_size_ex(HANDLE hfile, PLARGE_INTEGER file_size)
{
    FILE_WIN* new_file = (FILE_WIN*)hfile;
    return GetFileSizeEx(new_file->file_handle, file_size);
}

bool file_util_set_file_pointer_ex(HANDLE hFile, LARGE_INTEGER distance_to_move, PLARGE_INTEGER new_file_pointer, DWORD move_method)
{
    FILE_WIN* new_file = (FILE_WIN*)hFile;
    return SetFilePointerEx(new_file->file_handle, distance_to_move, new_file_pointer, move_method);
}

bool file_util_set_end_of_file(HANDLE hfile)
{
    FILE_WIN* new_file = (FILE_WIN*)hfile;
    return SetEndOfFile(new_file->file_handle);
}

bool file_util_set_file_valid_data(HANDLE hfile, LONGLONG valid_data_length)
{
    FILE_WIN* new_file = (FILE_WIN*)hfile;
    return SetFileValidData(new_file->file_handle, valid_data_length);
}