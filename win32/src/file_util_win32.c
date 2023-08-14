// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep

#include "c_logging/logger.h"
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
        LogError("Invalid arguments to file_util_open_file: full_file_name = %s, uint32_t access = %p, uint32_t share_mode = %p, LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %p, uint32_t flags_and_attributes = %p, HANDLE template_file = %p",
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
            new_file->ptpp = CreateThreadpool(NULL);
            new_file->ptpcg = CreateThreadpoolCleanupGroup();
        }
        return new_file;
    }
}

bool file_util_close_file(HANDLE handle_input)
{
    if(handle_input == NULL)
    {
        LogError("Invalid arguement to file_util_close_handle: Hanle input was an INVALID_HANDLE_VALUE");
        return false;
    }
    else
    {
        FILE_WIN* new_file = (FILE_WIN*)handle_input;
        return CloseHandle(new_file->file_handle);
    }
}

bool file_util_write_file(HANDLE handle_input, LPCVOID buffer, uint32_t number_of_bytes_to_write, LPOVERLAPPED overlapped)
{
    if(handle_input == NULL || buffer == NULL || number_of_bytes_to_write == 0)
    {
        LogError("Invalid inputs to file_util_write_file: HANDLE handle_in = %p, LPCVOID buffer = %p, uint32_t number_of_bytes_to_write = %p",
                    handle_input, buffer, number_of_bytes_to_write);
        return false;
    }
    else
    {
        FILE_WIN* new_file = (FILE_WIN*)handle_input;
        return WriteFile(new_file->file_handle, buffer, number_of_bytes_to_write, NULL, overlapped);
    }


}

bool file_util_delete_file(LPCSTR full_file_name)
{
    if(full_file_name == NULL || full_file_name[0] == '\0'){
        LogError("Invalid file name/path: LPCSTR full_fil_name = %s",
            full_file_name);
        return false;
    }
    else
    {
        return DeleteFileA(full_file_name);
    }
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