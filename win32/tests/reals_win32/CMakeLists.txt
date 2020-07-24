#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)


set(reals_win32_c_files
    real_threadapi.c
    real_interlocked_hl_win32.c
    real_srw_lock_win32.c
    real_string_utils_win32.c
    real_timer_win32.c
    real_interlocked.c
)

set(reals_win32_h_files
    real_interlocked.h
    real_interlocked_renames.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(reals_win32 ${reals_win32_c_files} ${reals_win32_h_files})
target_include_directories(reals_win32 PUBLIC .)
target_link_libraries(reals_win32 reals_pal_interface)
