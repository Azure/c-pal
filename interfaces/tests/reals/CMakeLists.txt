#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(reals_pal_interface_h_files
    real_threadapi.h
    real_threadapi_renames.h
)

if(WIN32)
set(reals_pal_interface_h_files ${reals_pal_interface_h_files}
    real_interlocked_hl.h
    real_interlocked_hl_renames.h
    real_srw_lock.h
    real_srw_lock_renames.h
    real_string_utils.h
    real_string_utils_renames.h
    real_timer.h
    real_timer_renames.h
)
else()
endif()

add_library(reals_pal_interface INTERFACE)
target_include_directories(reals_pal_interface INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(reals_pal_interface INTERFACE azure_c_logging pal_interfaces)
