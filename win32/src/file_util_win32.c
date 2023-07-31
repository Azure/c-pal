// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep

#include "c_logging/logger.h"
#include "c_pal/interlocked.h"

#include "c_pal/file_util.h"


HANDLE file_util_open_file(const char* full_file_name, uint32_t access, uint32_t share_mode, LPSECURITY_ATTRIBUTES security_attributes, 
                    uint32_t creation_disposition, uint32_t flags_and_attributes, HANDLE template_file)
{
    return CreateFileA(full_file_name, access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);

}

bool file_util_close_file(HANDLE handle_input)
{
    return CloseHandle(handle_input);
}

bool file_util_write_file(HANDLE handle_input, LPCVOID buffer, uint32_t number_of_bytes_to_write, LPOVERLAPPED overlapped,
                    LPOVERLAPPED_COMPLETION_ROUTINE completion_routine)
{
    return WriteFileEx(handle_input, buffer, number_of_bytes_to_write, overlapped, completion_routine);
}

bool file_util_delete_file(LPCSTR full_file_name)
{
    return DeleteFileA(full_file_name);
}