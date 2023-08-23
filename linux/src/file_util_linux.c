// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_logging/xlogging.h"

#include "c_pal/windows_defines.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_ll.h"
#include "c_pal/thandle.h"
#include "c_pal/file_util.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/error_handling_linux.h"

typedef struct CREATE_FILE_LINUX_TAG
{
    int h_file;
    const char* full_file_name;
    PTP_IO ptp_io;
    PTP_WIN32_IO_CALLBACK_FUNC callback_func;
    PVOID pv;
    UCHAR flags;
    THANDLE(THREADPOOL) threadpool;
    int file_pointer_pos;
} CREATE_FILE_LINUX;

typedef struct WRITE_FILE_LINUX_TAG
{
    CREATE_FILE_LINUX* handle_input;
    LPCVOID buffer;
    uint32_t number_of_bytes_to_write;
    LPOVERLAPPED overlapped;
    LPOVERLAPPED_COMPLETION_ROUTINE completion_routine;
    PTP_WIN32_IO_CALLBACK_FUNC callback_func;
    PVOID pv;

} WRITE_FILE_LINUX;

HANDLE file_util_open_file(const char* full_file_name, uint32_t access, uint32_t share_mode, LPSECURITY_ATTRIBUTES security_attributes, 
                    uint32_t creation_disposition, uint32_t flags_and_attributes, HANDLE template_file)
{
    CREATE_FILE_LINUX* result;
    if((full_file_name == NULL) || (full_file_name[0] == '\0'))
    {
        /*Codes_SRS_FILE_UTIL_LINUX_09_001: [ If the full_file_name input is either empty or NULL, file_util_open_file shall fail and return an INVALID_HANDLE_VALUE. ]*/
        LogError("Invalid arguments to file_util_open_file: full_file_name = %s, uint32_t access = %u, uint32_t share_mode = %u, LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %u, uint32_t flags_and_attributes = %u, HANDLE template_file = %p",
                    full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
        result = INVALID_HANDLE_VALUE;
    } 
    else 
    {
        bool threadpool_success = true;
        result = malloc(sizeof(CREATE_FILE_LINUX));
        if(result == NULL)
        {
            /*Codes_SRS_FILE_UTIL_LINUX_09_008: [ If there are any failures, file_util_open_file shall fail and return INVALID_HANDLE_VALUE. ]*/
            LogError("Failure in malloc");
            result = INVALID_HANDLE_VALUE;
        }
        else
        {

        if(flags_and_attributes & FILE_FLAG_OVERLAPPED)
        {
            EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
            THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);

            if(threadpool == NULL)
            {
                LogError("Failure in creating threadpool, full_file_name = %s, uint32_t access = %u, uint32_t share_mode = %u, LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %u, uint32_t flags_and_attributes = %u, HANDLE template_file = %p",
                    full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
                result = INVALID_HANDLE_VALUE;
                threadpool_success = false;
            }
            else
            {
                threadpool_open(threadpool);
                if(threadpool != NULL)
                {
                    THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, threadpool);
                }
            }
        }
        /*Codes_SRS_FILE_UTIL_LINUX_09_002: [ file_util_open_file shall allocate memory for the file handle. ]*/
        if(threadpool_success == true)
        {
                int user_access;
                int result_creation_disposition;

                if (access == GENERIC_READ)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_004: [ If desired_access is GENERIC_READ, file_util_open_file shall call open with O_RDONLY and shall return a file handle for read only. ]*/
                    user_access = O_RDONLY;
                } 
                else if (access == GENERIC_WRITE)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_005: [ If desired_access is GENERIC_WRITE, file_util_open_file shall call open with O_WRONLY and shall return a file handle for write only. ]*/
                    user_access = O_WRONLY;
                } 
                else if (access == GENERIC_ALL || access == (GENERIC_READ&GENERIC_WRITE))
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_006: [ If desired_access is GENERIC_ALL or GENERIC_READ&GENERIC_WRITE, file_util_open_file shall call open with O_RDWR and shall return a file handle for read and write. ]*/
                    user_access = O_RDWR;
                }
                else
                {
                    user_access = 0;
                }
            
                if (creation_disposition == CREATE_ALWAYS || creation_disposition == OPEN_ALWAYS)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_014: [ If creation_disposition is CREATE_ALWAYS or OPEN_ALWAYS, file_util_open_file shall call open with O_CREAT and shall either create a new file handle if the specificied pathname exists and return it or return an existing file handle. ]*/
                    result_creation_disposition = O_CREAT;
                }
                else if (creation_disposition == CREATE_NEW)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_016: [ If creation_disposition is CREATE_NEW and the file already exists, file_util_open_file shall fail and return INVALID_HANDLE_VALUE. ]*/
                    /*Codes_SRS_FILE_UTIL_LINUX_09_015: [ If creation_disposition is CREATE_NEW, file_util_open_file shall call open with O_CREAT|O_EXCL and shall return a new file handle if the file doesn't already exist. ]*/
                    result_creation_disposition = O_CREAT|O_EXCL;
                }
                else if (creation_disposition == TRUNCATE_EXISTING)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_017: [ If creation_disposition is TRUNCATE_EXISTING, file_util_open_file shall call open with O_TRUNC and shall return a file handle whose size has been truncated to zero bytes. ]*/
                    result_creation_disposition = O_TRUNC;
                }
                else
                {
                    result_creation_disposition = 0;
                }

                int flags = user_access|result_creation_disposition;

                /*Codes_SRS_FILE_UTIL_LINUX_09_020: [ file_util_open_file shall succeed and return a non-NULL value. ]*/
                result->h_file = open(full_file_name, flags);
                result->full_file_name = full_file_name;
                if(result->h_file == -1)
                {
                    /*Codes_SRS_FILE_UTIL_LINUX_09_008: [ If there are any failures, file_util_open_file shall fail and return INVALID_HANDLE_VALUE. ]*/
                    LogError("Failure in creating a file, full_file_name = %s, uint32_t access = %u, uint32_t share_mode = %u, LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %u, uint32_t flags_and_attributes = %u, HANDLE template_file = %p",
                        full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
                    free(result);
                    result = INVALID_HANDLE_VALUE;
                }
        }
        }
        
    }
    return result;
}

bool file_util_close_file(HANDLE handle_input)
{
    bool result;

    if(handle_input == NULL)
    {   
        /*Codes_SRS_FILE_UTIL_LINUX_09_021: [ If handle_input is NULL, file_util_close_file shall fail and return false. ]*/
        result = false;
        LogError("Invalid argument to file_util_close_file. Handle input was an INVALID_HANDLE_VALUE");
    }
    else 
    {
        CREATE_FILE_LINUX* cfl = (CREATE_FILE_LINUX*)handle_input;

        /*Codes_SRS_FILE_UTIL_LINUX_09_019: [ file_util_close_file shall call close on the given handle_input. ]*/
        if(close(cfl->h_file) != 0)
        {
            /*Codes_SRS_FILE_UTIL_LINUX_09_009: [ If there are any failures, `file_util_close_file` shall fail and return false. ]*/
            result = false;
            LogError("Failure in closing file");
        }
        else 
        {
            /*Codes_SRS_FILE_UTIL_LINUX_09_018: [ file_util_close_file shall succeed and return true. ]*/
            result = true;
        }
        free(cfl);
    }

    return result;
}

PTP_CLEANUP_GROUP file_util_create_threadpool_cleanup_group()
{
    return NULL;
}

static void file_util_work_function(void* context)
{
    if(context == NULL)
    {
        LogError("Invalid inputs to file_util_work_function");
    }
    else
    {
        WRITE_FILE_LINUX* tp_input = (WRITE_FILE_LINUX*)context;
        uint64_t Io_Result = NO_ERROR;
        if(tp_input == NULL)
        {
            LogError("Failure in malloc");
        }
        else
        {
            ssize_t write_success;

            write_success = write(tp_input->handle_input->h_file, tp_input->buffer, tp_input->number_of_bytes_to_write);

            if(write_success == -1)
            {
                LogError("Failed to write to file");
                error_handling_linux_set_last_error(ERROR_WRITE_FAULT);
                Io_Result = error_handling_linux_get_last_error();
            }
            else
            {
                tp_input->callback_func(NULL, tp_input->pv, tp_input->overlapped, Io_Result, tp_input->number_of_bytes_to_write, NULL);
                free(tp_input);
            }
        }
    }

}

bool file_util_write_file(HANDLE handle_in, LPCVOID buffer, uint32_t number_of_bytes_to_write, 
                    LPOVERLAPPED overlapped)
{
    bool success_write;
    if(handle_in == NULL || buffer == NULL || number_of_bytes_to_write == 0)
    {
        LogError("Invalid inputs to file_util_write_file: HANDLE handle_in = %p, LPCVOID buffer = %p, uint32_t number_of_bytes_to_write = %u",
                    handle_in, buffer, number_of_bytes_to_write);
        success_write = false;
    }
    else
    {
        CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)handle_in;

        WRITE_FILE_LINUX* tp_input = malloc(sizeof(WRITE_FILE_LINUX));
        if(tp_input == NULL)
        {
            LogError("Failure in malloc");
            success_write = false;
        }
        else
        {
            tp_input->handle_input = handle_input;
            tp_input->buffer = buffer;
            tp_input->number_of_bytes_to_write = number_of_bytes_to_write;
            tp_input->overlapped = overlapped;
            tp_input->callback_func = handle_input->callback_func;
            tp_input->pv = handle_input->pv;
            if(overlapped == NULL)
            {
                file_util_work_function(tp_input);
                success_write = true;
            }
            else
            {
                threadpool_schedule_work(handle_input->threadpool, file_util_work_function, tp_input);
                success_write = true;
            }
        }
    }
    return success_write;
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
        int success = unlink(full_file_name);
        if(success == -1)
        {
            return false;
            LogError("Failed to delete file: LPCSTR full_file_name = %s",
                    full_file_name);
        }
        else
        {
            return true;
        }
    }
}

PTP_IO file_util_create_threadpool_io(HANDLE handle_in, PTP_WIN32_IO_CALLBACK pfnio_in, PVOID pv)
{
    if(handle_in == NULL || pfnio_in == NULL)
    {
        LogError("Invalid inputs to file_util_create_threadpool: HANDLE handle_input = %p, PTP_WIN32_IO_CALLBACK = %p",
                    handle_in, pfnio_in);
        return NULL;
    }
    else
    {
        void (*pfnio)() = pfnio_in;
        CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)handle_in;
        handle_input->callback_func = pfnio;
        handle_input->pv = pv;
        return handle_input;
    }
}

bool file_util_set_file_completion_notifcation_modes(HANDLE handle_input, UCHAR flags)
{
    CREATE_FILE_LINUX* file_handle = (CREATE_FILE_LINUX*)handle_input;
    file_handle->flags = flags;

    return true;
}

HANDLE file_util_create_event(LPSECURITY_ATTRIBUTES lpEventAttributes, bool bManualReset, bool bInitialState, LPCSTR lpName)
{
    return NULL;
}

bool file_util_query_performance_counter(LARGE_INTEGER* performance_count)
{
    return true;
}

void file_util_cancel_threadpool_io(PTP_IO pio)
{
    (void)pio;
}

bool file_util_read_file(HANDLE handle_in, LPVOID buffer, DWORD number_of_bytes_to_read, LPDWORD number_of_bytes_read, LPOVERLAPPED overlapped)
{
    if(handle_in == INVALID_HANDLE_VALUE || buffer == NULL || number_of_bytes_to_read == 0)
    {
        LogError("Invalid inputs to file_util_read_file: HANDLE handle_in = %p, LPVOID buffer = %p, LPDWORD number_of_bytes_read = %p",
                    handle_in, buffer, number_of_bytes_read);
        return false;
    }
    else
    {
        CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)handle_in;

        int success = read(handle_input->h_file, buffer, number_of_bytes_to_read);
        if(success == -1)
        {
            LogError("Failure in reading file");
            return false;
        }
        else
        {
            return true;
        }
    }
}

bool file_util_set_file_information_by_handle(HANDLE handle_in, FILE_INFO_BY_HANDLE_CLASS file_info_class, LPVOID file_info, DWORD buffer_size)
{
    CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)handle_in;
    if(file_info_class == FileRenameInfo)
    {
        FILE_RENAME_INFO* rename_info = (FILE_RENAME_INFO*)file_info;
        int success = rename(handle_input->full_file_name, (const char*)rename_info->FileName);
        if(success == 0)
        {
            return true;
        }
        else
        {
            LogError("Failure in renaming file");
            return false;
        }
    }
    else
    {
        return true;
    }
}

void file_util_close_threadpool_cleanup_group(PTP_CLEANUP_GROUP ptpcg)
{
    (void)ptpcg;
}

void file_util_destroy_threadpool_environment(PTP_CALLBACK_ENVIRON pcbe)
{
    (void)pcbe;
}

void file_util_close_threadpool_io(PTP_IO pio)
{
    (void)pio;
}

bool file_util_get_file_size_ex(HANDLE hfile, PLARGE_INTEGER file_size)
{
    CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)hfile;
    struct stat st;
    stat(handle_input->full_file_name, &st);
    file_size->QuadPart = st.st_size;
    if(&file_size->QuadPart == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool file_util_set_file_pointer_ex(HANDLE hFile, LARGE_INTEGER distance_to_move, PLARGE_INTEGER new_file_pointer, DWORD move_method)
{
    //CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)hFile;
    //char* mode = "r";
    // FILE* temp_file = fopen(handle_input->full_file_name, mode);
    FILE* temp_file = (FILE*)hFile;
    int whence;
    if(move_method == FILE_BEGIN)
    {
        whence = SEEK_SET;
    }
    else if (move_method == FILE_CURRENT)
    {
        whence = SEEK_CUR;
    }
    else if (move_method == FILE_END)
    {
        whence = SEEK_END;
    }
    long move = distance_to_move.QuadPart;
    int success = fseek(temp_file, move, whence);
    //fclose(temp_file);
    if(success == -1)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool file_util_set_end_of_file(HANDLE hfile)
{
    //CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)hfile;
    FILE* temp_file = (FILE*)hfile;
    int file_pointer_pos = ftell(temp_file);
    if(file_pointer_pos == -1)
    {
        LogError("Unable to find file pointer position");
        return false;
    }
    else
    {
        CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)temp_file;
        int success = truncate(handle_input->full_file_name, file_pointer_pos);
        if(success == -1)
        {
            LogError("Unable to set end of file");
            return false;
        }
        else
        {
            return true;
        }
    }
}

bool file_util_set_file_valid_data(HANDLE hfile, LONGLONG valid_data_length)
{
    CREATE_FILE_LINUX* handle_input = (CREATE_FILE_LINUX*)hfile;
    int success = truncate(handle_input->full_file_name, valid_data_length);
    if(success == -1)
    {
        LogError("Unable to set end of file");
        return false;
    }
    else
    {
        return true;
    }
}