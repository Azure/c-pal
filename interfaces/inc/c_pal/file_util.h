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

MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, uint32_t, desired_access, uint32_t, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, uint32_t, creation_disposition, uint32_t, flags_and_attributes, HANDLE, template_file);
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);

#ifdef __cplusplus
}
#endif

#endif // FILE_UTIL_H
