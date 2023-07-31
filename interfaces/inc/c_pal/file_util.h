// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "c_pal/windows_defines.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_ll.h"
#include "c_pal/thandle.h"
#include "c_pal/file_util.h"

typedef struct CREATE_FILE_LINUX_TAG CREATE_FILE_LINUX;

MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, uint32_t, desired_access, uint32_t, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, uint32_t, creation_disposition, uint32_t, flags_and_attributes, HANDLE, template_file);
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_write_file, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write, LPOVERLAPPED, overlapped, LPOVERLAPPED_COMPLETION_ROUTINE, completion_routine);
MOCKABLE_FUNCTION(, bool, file_util_delete_file, LPCSTR, full_file_name);

#ifdef __cplusplus
}
#endif

#endif // FILE_UTIL_H
