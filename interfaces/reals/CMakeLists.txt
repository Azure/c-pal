#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(reals_pal_interface_h_files
    real_pipe.h
    real_pipe_renames.h
    real_threadapi.h
    real_threadapi_renames.h
)

if(WIN32)
set(reals_pal_interface_h_files ${reals_pal_interface_h_files}
    real_gballoc_ll.h
    real_gballoc_ll_renames.h
    real_gballoc_hl.h
    real_gballoc_hl_renames.h
    real_interlocked_hl.h
    real_interlocked_hl_renames.h
    real_srw_lock.h
    real_srw_lock_renames.h
    real_string_utils.h
    real_string_utils_renames.h
    real_threadapi_renames.h
    real_threadapi.h
    real_timer.h
    real_timer_renames.h
)
else()
endif()

add_library(pal_interfaces_reals INTERFACE)
target_include_directories(pal_interfaces_reals INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pal_interfaces_reals INTERFACE c_logging)
