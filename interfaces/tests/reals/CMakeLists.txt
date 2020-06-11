#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(azure_c_pal_reals_h_files
    real_threadapi.h
    real_threadapi_renames.h
)

if(WIN32)
set(azure_c_pal_reals_c_files ${azure_c_pal_reals_c_files}
    ../../../win32/src/reals_win32/real_interlocked_hl_win32.c
    ../../../win32/src/reals_win32/real_srw_lock_win32.c
    ../../../win32/src/reals_win32/real_string_utils_win32.c
    ../../../win32/src/reals_win32/real_timer_win32.c
)

set(azure_c_pal_reals_h_files ${azure_c_pal_reals_h_files}
    real_interlocked_hl.h
    real_interlocked_hl_renames.h
    real_srw_lock.h
    real_srw_lock_renames.h
    real_string_utils.h
    real_string_utils_renames.h
    real_timer.h
    real_timer_renames.h
)
set(REALS_SRC_DIR ../../../win32/src/reals_win32)
else()
set(REALS_SRC_DIR ../../../linux/src/reals_linux)
endif()



include_directories(${REALS_SRC_DIR})
add_library(azure_c_pal_reals ${azure_c_pal_reals_c_files} ${azure_c_pal_reals_h_files})
target_include_directories(azure_c_pal_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals>)
target_link_libraries(azure_c_pal_reals azure_c_logging)
