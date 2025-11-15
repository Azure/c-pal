// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_logging/logger.h"

#include "c_pal/windows_defines.h"

#include "c_pal/file_util.h"

typedef struct CREATE_FILE_LINUX_TAG
{
    int h_file;

} CREATE_FILE_LINUX;

HANDLE file_util_open_file(const char* full_file_name, uint32_t access, uint32_t share_mode, LPSECURITY_ATTRIBUTES security_attributes, 
                    uint32_t creation_disposition, uint32_t flags_and_attributes, HANDLE template_file)
{
    CREATE_FILE_LINUX* result;
    if((full_file_name == NULL) || (full_file_name[0] == '\0'))
    {
        /*Codes_SRS_FILE_UTIL_LINUX_09_001: [ If the full_file_name input is either empty or NULL, file_util_open_file shall return an INVALID_HANDLE_VALUE. ]*/
        LogError("Invalid arguments to file_util_open_file: full_file_name = %s, uint32_t access = %" PRIu32 ", uint32_t share_mode = %" PRIu32 ", LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %" PRIu32 ", uint32_t flags_and_attributes = %" PRIu32 ", HANDLE template_file = %p",
                    full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
        result = INVALID_HANDLE_VALUE;
    } 
    else 
    {
        /*Codes_SRS_FILE_UTIL_LINUX_09_002: [ file_util_open_file shall allocate memory for the file handle. ]*/
        result = malloc(sizeof(CREATE_FILE_LINUX));
        if(result == NULL)
        {
            /*Codes_SRS_FILE_UTIL_LINUX_09_008: [ If there are any failures, file_util_open_file shall fail and return INVALID_HANDLE_VALUE. ]*/
            LogError("Failure in malloc");
            result = INVALID_HANDLE_VALUE;
        }
        else 
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
            if(result->h_file == -1)
            {
                /*Codes_SRS_FILE_UTIL_LINUX_09_008: [ If there are any failures, file_util_open_file shall fail and return INVALID_HANDLE_VALUE. ]*/
                LogError("Failure in creating a file, full_file_name = %s, uint32_t access = %" PRIu32 ", uint32_t share_mode = %" PRIu32 ", LPSECURITY_ATTRIBUTES security_attributes = %p, uint32_t creation_disposition = %" PRIu32 ", uint32_t flags_and_attributes = %" PRIu32 ", HANDLE template_file = %p",
                    full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
                free(result);
                result = INVALID_HANDLE_VALUE;
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
            /*Codes_SRS_FILE_UTIL_LINUX_09_009: [ If there are any failures, file_util_close_file shall fail and return false. ]*/
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
