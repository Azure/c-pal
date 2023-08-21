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
MOCKABLE_FUNCTION(, bool, file_util_write_file, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write, LPOVERLAPPED, overlapped);
MOCKABLE_FUNCTION(, bool, file_util_delete_file, LPCSTR, full_file_name);
MOCKABLE_FUNCTION(, PTP_IO, file_util_create_threadpool_io, HANDLE, handle_input, PTP_WIN32_IO_CALLBACK, pfnio, PVOID, pv);
MOCKABLE_FUNCTION(, PTP_CLEANUP_GROUP, file_util_create_threadpool_cleanup_group);
MOCKABLE_FUNCTION(, bool, file_util_set_file_completion_notifcation_modes, HANDLE, handle_input, UCHAR, flags);
MOCKABLE_FUNCTION(, HANDLE, file_util_create_event, LPSECURITY_ATTRIBUTES, lpEventAttributes, bool, bManualReset, bool, bInitialState, LPCSTR, lpName);
MOCKABLE_FUNCTION(, bool, file_util_query_performance_counter, LARGE_INTEGER*, performance_count);
MOCKABLE_FUNCTION(, void, file_util_cancel_threadpool_io, PTP_IO, pio);
MOCKABLE_FUNCTION(, bool, file_util_read_file, HANDLE, handle_in, LPVOID, buffer, DWORD, number_of_bytes_to_read, LPDWORD, number_of_bytes_read, LPOVERLAPPED, overlapped);
MOCKABLE_FUNCTION(, bool, file_util_set_file_information_by_handle, HANDLE, handle_in, FILE_INFO_BY_HANDLE_CLASS, file_info_class, LPVOID, file_info, DWORD, buffer_size);
MOCKABLE_FUNCTION(, void, file_util_close_threadpool_cleanup_group, PTP_CLEANUP_GROUP, ptpcg);
MOCKABLE_FUNCTION(, void, file_util_destroy_threadpool_environment, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, file_util_close_threadpool_io, PTP_IO, pio);
MOCKABLE_FUNCTION(, bool, file_util_get_file_size_ex, HANDLE, hfile, PLARGE_INTEGER, file_size);
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, hFile, LARGE_INTEGER, DistanceToMove, PLARGE_INTEGER, new_file_pointer, DWORD, move_method);
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, hFile);
MOCKABLE_FUNCTION(, bool, file_util_set_file_valid_data, HANDLE, hFile, LONGLONG, valid_data_length);

#ifdef __cplusplus
}
#endif

#endif // FILE_UTIL_H
