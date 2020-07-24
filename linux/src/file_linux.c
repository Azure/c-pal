// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>



#include "c_logging/xlogging.h"
#include "c_pal/file.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/gballoc.h"

typedef struct FILE_HANDLE_DATA_TAG
{
    FILE_REPORT_FAULT user_report_fault_callback;
    void* user_report_fault_context;
    int h_file;
    volatile_atomic int32_t pending_io;
}FILE_HANDLE_DATA;

typedef struct FILE_LINUX_WRITE_TAG
{
    FILE_HANDLE handle;
    FILE_CB user_callback;
    struct aiocb* aiocbp;
    void* user_context;
    uint32_t size;
}FILE_LINUX_WRITE;

typedef struct FILE_LINUX_READ_TAG
{
    FILE_HANDLE handle;
    FILE_CB user_callback;
    struct aiocb* aiocbp;
    void* user_context;
    uint32_t size;
}FILE_LINUX_READ;

static void on_file_write_complete_linux(sigval_t sigval)
{
    FILE_LINUX_WRITE* write_info = (FILE_LINUX_WRITE*)sigval.sival_ptr;
    uint32_t size = write_info->size;

    /*Codes_SRS_FILE_LINUX_43_058: [ on_file_write_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in write_info. ]*/
    (void)interlocked_decrement(&write_info->handle->pending_io);
    /*Codes_SRS_FILE_LINUX_43_066: [ on_file_write_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
    wake_by_address_all(&write_info->handle->pending_io);

    /*Codes_SRS_FILE_LINUX_43_022: [ on_file_write_complete_linux shall call aio_return to determine if the asynchronous write operation succeeded. ]*/
    int return_val = aio_return(write_info->aiocbp);
    FILE_CB user_callback = write_info->user_callback;
    void* user_context = write_info->user_context;

    /*Codes_SRS_FILE_LINUX_43_062: [ on_file_write_complete_linux shall free the aiocb struct associated with the current asynchronous write operation. ]*/
    free(write_info->aiocbp);
    /*Codes_SRS_FILE_LINUX_43_063: [ on_file_write_complete_linux shall free write_info. ]*/
    free(write_info);

    bool succeeded;
    if (return_val == -1)
    {
        /*Codes_SRS_FILE_LINUX_43_023: [ If the asynchronous write operation did not succeed, on_file_io_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
        succeeded = false;
        LogError("Error in asynchronous write operation, errno=%d", errno);
    }
    else if ((uint32_t)return_val != size)
    {
        /*Codes_SRS_FILE_LINUX_43_064: [ If the number of bytes written are less than the bytes requested by the user, on_file_write_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
        succeeded = false;
        LogError("All bytes were not written.");
    }
    else
    {
        /*Codes_SRS_FILE_LINUX_43_027: [ If the asynchronous write operation succeeded, on_file_write_complete_linux shall call user_callback with user_context and true as is_successful. ]*/
        succeeded = true;
    }
    user_callback(user_context, succeeded);
}

static void on_file_read_complete_linux(sigval_t sigval)
{
    FILE_LINUX_READ* read_info = (FILE_LINUX_READ*)sigval.sival_ptr;
    uint32_t size = read_info->size;

    /*Codes_SRS_FILE_LINUX_43_059: [ on_file_read_complete_linux shall decrement the pending asynchronous operations counter on the FILE_HANDLE contained in read_info. ]*/
    (void)interlocked_decrement(&read_info->handle->pending_io);
    /*Codes_SRS_FILE_LINUX_43_067: [ on_file_read_complete_linux shall wake all threads waiting on the address of the pending asynchronous operations counter. ]*/
    wake_by_address_all(&read_info->handle->pending_io);

    /*Codes_SRS_FILE_LINUX_43_040: [ on_file_read_complete_linux shall call aio_return to determine if the asynchronous read operation succeeded. ]*/
    int return_val = aio_return(read_info->aiocbp);
    FILE_CB user_callback = read_info->user_callback;
    void* user_context = read_info->user_context;

    /*Codes_SRS_FILE_LINUX_43_060: [ on_file_read_complete_linux shall free the aiocb struct associated with the current asynchronous read operation. ]*/
    free(read_info->aiocbp);
    /*Codes_SRS_FILE_LINUX_43_061: [ on_file_read_complete_linux shall free read_info. ]*/
    free(read_info);

    bool succeeded;
    if (return_val == -1)
    {
        /*Codes_SRS_FILE_LINUX_43_041: [ If the asynchronous read operation did not succeed, on_file_io_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
        succeeded = false;
        LogError("Error in asynchronous read operation, errno=%d", errno);
    }
    else if ((uint32_t)return_val != size)
    {
        /*Codes_SRS_FILE_LINUX_43_065: [ If the number of bytes read are less than the bytes requested by the user, on_file_read_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
        succeeded = false;
        LogError("All bytes were not read.");
    }
    else
    {
        /*Codes_SRS_FILE_LINUX_43_042: [ If the asynchronous read operation succeeded, on_file_read_complete_linux shall call user_callback with user_context and false as is_successful. ]*/
        succeeded = true;
    }
    user_callback(user_context, succeeded);
}

IMPLEMENT_MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context)
{
    (void)execution_engine;
    FILE_HANDLE result;
    if (
        /*Codes_SRS_FILE_43_002: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
        /*Codes_SRS_FILE_LINUX_43_038: [ If full_file_name is NULL then file_create shall fail and return NULL. ]*/
        (full_file_name == NULL) ||
        /*Codes_SRS_FILE_43_037: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
        /*Codes_SRS_FILE_LINUX_43_050: [ If full_file_name is an empty string, file_create shall fail and return NULL. ]*/
        (full_file_name[0] == '\0')
        )
    {
        LogError("Invalid arguments to file_create: EXECUTION_ENGINE_HANDLE execution_engine=%p, const char* full_file_name=%s, FILE_REPORT_FAULT user_report_callback=%p, void* user_report_faul_context=%p",
            execution_engine, MU_P_OR_NULL(full_file_name), user_report_fault_callback, user_report_fault_context);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_FILE_LINUX_43_029: [ file_create shall allocate a FILE_HANDLE. ]*/
        result = malloc(sizeof(FILE_HANDLE_DATA));
        if (result == NULL)
        {
            /*Codes_SRS_FILE_43_034: [ If there are any failures, file_create shall fail and return NULL. ]*/
            /*Codes_SRS_FILE_LINUX_43_053: [ If there are any other failures, file_create shall fail and return NULL. ]*/
            LogError("Failure in malloc");
        }
        else
        {
            /*Codes_SRS_FILE_43_003: [ If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
            /*Codes_SRS_FILE_LINUX_43_001: [ file_create shall call open with full_file_name as pathname and flags O_CREAT, O_RDWR, O_DIRECT and O_LARGEFILE. ]*/
            result->h_file = open(full_file_name, O_CREAT | O_RDWR | __O_DIRECT | __O_LARGEFILE, 0700);
            if ( result->h_file == -1)
            {
                /*Codes_SRS_FILE_43_034: [ If there are any failures, file_create shall fail and return NULL. ]*/
                /*Codes_SRS_FILE_LINUX_43_053: [ If there are any other failures, file_create shall fail and return NULL. ]*/
                LogError("Failure in open, full_file_name=%s, errno=%d", full_file_name, errno);
                free(result);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_FILE_LINUX_43_002: [ file_create shall succeed and return a non-NULL value.]*/
                /*Codes_SRS_FILE_LINUX_43_055: [ file_create shall initialize a counter for pending asynchronous operations to 0. ]*/
                (void)interlocked_exchange(&result->pending_io, 0);
                result->user_report_fault_callback = user_report_fault_callback;
                result->user_report_fault_context = user_report_fault_context;
            }
        }
    }
    return result;
}


IMPLEMENT_MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle)
{
    
    if (handle == NULL)
    {
        /*Codes_SRS_FILE_43_005: [ If handle is NULL, file_destroy shall return. ]*/
        /*Codes_SRS_FILE_LINUX_43_036: [ If handle is NULL, file_destroy shall return. ]*/
        LogError("Invalid argument to file_destroy: FILE_HANDLE=%p", handle);
    }
    else
    {
        /*Codes_SRS_FILE_43_006: [ file_destroy shall wait for all pending I/O operations to complete. ]*/
        /*Codes_SRS_FILE_LINUX_43_054: [ file_destroy shall wait for the pending asynchronous operations counter to become equal to zero. ]*/
        int32_t pending = interlocked_add(&handle->pending_io, 0);
        while (pending != 0)
        {
            wait_on_address(&handle->pending_io, &pending, UINT32_MAX);
            pending = interlocked_add(&handle->pending_io, 0);
        }
        /*Codes_SRS_FILE_LINUX_43_003: [ file_destroy shall call close.]*/
        if (close(handle->h_file) == -1)
        {
            LogError("failure in close, FILE_HANDLE=%p, errno=%d", handle, errno);
        }
        /*Codes_SRS_FILE_LINUX_43_030: [ file_destroy shall free the FILE_HANDLE. ]*/
        free(handle);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)
{
    FILE_WRITE_ASYNC_RESULT result;
    if
    (
        /*Codes_SRS_FILE_43_009: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_031: [ If handle is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        (handle == NULL) ||
        /*Codes_SRS_FILE_43_010: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_032: [ If source is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        (source == NULL) ||
        /*Codes_SRS_FILE_43_012: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_049: [ If user_callback is NULL then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        (user_callback == NULL)||
        /*Codes_SRS_FILE_43_040: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_051: [ If position + size is greater than INT64_MAX, then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        ((position + size) > INT64_MAX)||
        /*Codes_SRS_FILE_43_042: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_048: [ If size is 0 then file_write_async shall fail and return FILE_WRITE_ASYNC_INVALID_ARGS. ]*/
        (size == 0)
    )
    {
        LogError("Invalid arguments to file_write_async: FILE_HANDLE file_handle=%p, const unsigned char* source=%p, uin32_t size=%" PRIu32 ", uint64_t position=%" PRIu64 ", FILE_CB user_callback=%p, void* user_context=%p",
            handle, source, size, position, user_callback, user_context);
        result = FILE_WRITE_ASYNC_INVALID_ARGS;
    }
    else
    {
        bool callback_will_be_called;
        /*Codes_SRS_FILE_LINUX_43_019: [ file_write_async shall allocate a struct to hold handle, source, size, user_callback and user_context. ]*/
        FILE_LINUX_WRITE* io_context = malloc(sizeof(FILE_LINUX_WRITE));
        if (io_context == NULL)
        {
            /*Codes_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
            LogError("failure in malloc");
            result = FILE_WRITE_ASYNC_ERROR;
        }
        else
        {
            io_context->handle = handle;
            io_context->size = size;
            io_context->user_callback = user_callback;
            io_context->user_context = user_context;

            /*Codes_SRS_FILE_LINUX_43_005: [ file_write_async shall allocate an aiocb struct with the file descriptor from file_handle as aio_fildes, position as aio_offset, source as aio_buf, size as aio_nbytes. ]*/
            io_context->aiocbp = malloc(sizeof(struct aiocb));
            if (io_context->aiocbp == NULL)
            {
                /*Codes_SRS_FILE_43_015: [ If there are any failures, file_write_async shall fail and return FILE_WRITE_ASYNC_ERROR. ]*/
                /*Codes_SRS_FILE_LINUX_43_013: [ If there are any other failures, file_write_async shall return FILE_WRITE_ASYNC_ERROR. ]*/
                LogError("Failure in malloc.");
                result = FILE_WRITE_ASYNC_ERROR;
            }
            else
            {
            
                (void)memset(io_context->aiocbp, 0, sizeof(struct aiocb));
                io_context->aiocbp->aio_fildes = handle->h_file;
                io_context->aiocbp->aio_buf = (unsigned char*)source;
                io_context->aiocbp->aio_nbytes = size;
                io_context->aiocbp->aio_offset = position;

                /*Codes_SRS_FILE_LINUX_43_004: [ file_write_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, the allocated struct as sigev_value, on_file_write_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
                (void)memset(&io_context->aiocbp->aio_sigevent, 0, sizeof(struct sigevent));
                io_context->aiocbp->aio_sigevent.sigev_notify = SIGEV_THREAD;
                io_context->aiocbp->aio_sigevent.sigev_value.sival_ptr = io_context;
                io_context->aiocbp->aio_sigevent.sigev_notify_function = on_file_write_complete_linux;
                io_context->aiocbp->aio_sigevent.sigev_notify_attributes = NULL;

                /*Codes_SRS_FILE_43_014: [ file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
                /*Codes_SRS_FILE_43_041: [ If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write. ]*/
                /*Codes_SRS_FILE_LINUX_43_006: [ file_write_async shall call aio_write with the allocated aiocb struct as aiocbp.]*/
                if (aio_write(io_context->aiocbp) == -1)
                {
                    /*Codes_SRS_FILE_43_035: [ If the call to write the file fails, file_write_async shall fail and return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
                    /*Codes_SRS_FILE_LINUX_43_012: [ If aio_write fails, file_write_async shall return FILE_WRITE_ASYNC_WRITE_ERROR. ]*/
                    LogError("Failure in aio_write, errno=%d", errno);
                    callback_will_be_called = false;
                    result = FILE_WRITE_ASYNC_WRITE_ERROR;
                }
                else
                {
                    /*Codes_SRS_FILE_43_008: [ file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
                    /*Codes_SRS_FILE_43_030: [ file_write_async shall succeed and return FILE_WRITE_ASYNC_OK. ]*/
                    /*Codes_SRS_FILE_LINUX_43_007: [ If aio_write succeeds, file_write_async shall return FILE_WRITE_ASYNC_OK. ]*/
                    /*Codes_SRS_FILE_LINUX_43_056: [ If aio_write succeeds, file_write_async shall increment the pending asynchronous operations counter on file_handle. ]*/
                    (void)interlocked_increment(&handle->pending_io);
                    callback_will_be_called = true;
                    result = FILE_WRITE_ASYNC_OK;
                }
                if (!callback_will_be_called)
                {
                    free(io_context->aiocbp);
                }
            }
            if (!callback_will_be_called)
            {
                free(io_context);
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
        /*Codes_SRS_FILE_LINUX_43_034: [ If handle is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (handle == NULL) ||
        /*Codes_SRS_FILE_43_032: [ If destination is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_043: [ IfdestinationisNULLthenfile_read_asyncshall fail and returnFILE_READ_ASYNC_INVALID_ARGS`. ]*/
        (destination == NULL) ||
        /*Codes_SRS_FILE_43_020: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_035: [ If user_callback is NULL then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (user_callback == NULL)||
        /*Codes_SRS_FILE_43_043: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        /*Codes_SRS_FILE_LINUX_43_052: [ If size is 0 then file_read_async shall fail and return FILE_READ_ASYNC_INVALID_ARGS. ]*/
        (size == 0)
    )
    {
        LogError("Invalid arguments to file_read_async: FILE_HANDLE file_handle=%p, unsigned char* destination=%p, uin32_t size=%" PRIu32 ", uint64_t position=%" PRIu64 ", FILE_CB user_callback=%p, void* user_context=%p",
            handle, destination, size, position, user_callback, user_context);
        result = FILE_READ_ASYNC_INVALID_ARGS;
    }
    else
    {
        bool callback_will_be_called;
        /*Codes_SRS_FILE_LINUX_43_045: [ file_read_async shall allocate a struct to hold handle, destination, user_callback and user_context. ]*/
        FILE_LINUX_READ* io_context = malloc(sizeof(FILE_LINUX_READ));
        if (io_context == NULL)
        {
            /*Codes_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
            /*Codes_SRS_FILE_LINUX_43_015: [ If there are any other failures, file_read_async shall return FILE_READ_ASYNC_ERROR. ]*/
            LogError("failure in malloc");
            callback_will_be_called = false;
            result = FILE_WRITE_ASYNC_ERROR;
        }
        else
        {
            io_context->handle = handle;
            io_context->size = size;
            io_context->user_callback = user_callback;
            io_context->user_context = user_context;
            
            /*Codes_SRS_FILE_LINUX_43_009: [ file_read_async shall allocate an aiocb struct with the file descriptor from file_handle as aio_fildes, position as aio_offset, the allocated buffer as aio_buf, size as aio_nbytes. ]*/
            io_context->aiocbp = malloc(sizeof(struct aiocb));
            if (io_context->aiocbp == NULL)
            {
                /*Codes_SRS_FILE_43_022: [ If there are any failures then file_read_async shall fail and return FILE_READ_ASYNC_ERROR. ]*/
                /*Codes_SRS_FILE_LINUX_43_015: [ If there are any other failures, file_read_async shall return FILE_READ_ASYNC_ERROR. ]*/
                LogError("failure in malloc");
                callback_will_be_called = false;
                result = FILE_WRITE_ASYNC_ERROR;
            }
            else
            {
                (void)memset(io_context->aiocbp, 0, sizeof(struct aiocb));
                io_context->aiocbp->aio_fildes = handle->h_file;
                io_context->aiocbp->aio_buf = destination;
                io_context->aiocbp->aio_nbytes = size;
                io_context->aiocbp->aio_offset = position;

                /*Codes_SRS_FILE_LINUX_43_008: [ file_read_async shall initialize the sigevent struct on the allocated aiocb struct with SIGEV_THREAD as sigev_notify, user_context as sigev_value, on_file_read_complete_linux as sigev_notify_function, NULL as sigev_notify_attributes. ]*/
                (void)memset(&io_context->aiocbp->aio_sigevent, 0, sizeof(struct sigevent));
                io_context->aiocbp->aio_sigevent.sigev_notify = SIGEV_THREAD;
                io_context->aiocbp->aio_sigevent.sigev_value.sival_ptr = io_context;
                io_context->aiocbp->aio_sigevent.sigev_notify_function = on_file_read_complete_linux;
                io_context->aiocbp->aio_sigevent.sigev_notify_attributes = NULL;

                /*Codes_SRS_FILE_43_021: [ file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
                /*Codes_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
                /*Codes_SRS_FILE_LINUX_43_010: [ file_read_async shall call aio_read with the allocated aiocb struct as aiocbp. ]*/
                if (aio_read(io_context->aiocbp) == -1)
                {
                    /*Codes_SRS_FILE_43_036: [ If the call to read the file fails, file_read_async shall fail and return FILE_READ_ASYNC_READ_ERROR. ]*/
                    /*Codes_SRS_FILE_LINUX_43_011: [ If aio_read fails, file_read_async shall return FILE_READ_ASYNC_READ_ERROR. ]*/
                    LogError("Failure in aio_read, errno=%d", errno);
                    callback_will_be_called = false;
                    result = FILE_READ_ASYNC_READ_ERROR;
                }
                else
                {
                    /*Codes_SRS_FILE_43_016: [ file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
                    /*Codes_SRS_FILE_43_031: [ file_read_async shall succeed and return FILE_READ_ASYNC_OK. ]*/
                    /*Codes_SRS_FILE_LINUX_43_057: [ If aio_read succeeds, file_read_async shall increment the pending asynchronous operations counter on file_handle. ]*/
                    /*Codes_SRS_FILE_LINUX_43_014: [ If aio_read succeeds, file_read_async shall return FILE_READ_ASYNC_OK. ]*/
                    (void)interlocked_increment(&handle->pending_io);
                    callback_will_be_called = true;
                    result = FILE_READ_ASYNC_OK;
                }
                if (!callback_will_be_called)
                {
                    free(io_context->aiocbp);
                }
            }
            if (!callback_will_be_called)
            {
                free(io_context);
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)
{
    (void)handle;
    (void)desired_size;
    /*Codes_SRS_FILE_LINUX_43_018: [ file_extend shall return 0. ]*/
    return 0;
}

