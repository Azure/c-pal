// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#endif
#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32 
#include "windows.h"
#else
#include "c_pal/windows_defines.h"

MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, template_file);
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_get_file_size, PLARGE_INTEGER*, file_size, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, handle_input, LARGE_INTEGER, distance_to_move, PLARGE_INTEGER, new_file_pointer, uint32_t, move_method);
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_write_file_ex, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write);

#ifdef __cplusplus
}
#endif

#endif // FILE_UTIL_H
