#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(reals_win32_ll_c_files
    real_interlocked.c
    real_sync.c
)

set(reals_win32_ll_h_files
    real_interlocked.h
    real_interlocked_renames.h
)

add_library(win32_ll_reals ${reals_win32_ll_c_files} ${reals_win32_ll_h_files})
set_target_properties(win32_ll_reals PROPERTIES LINKER_LANGUAGE C)
target_include_directories(win32_ll_reals PUBLIC .  ${CMAKE_CURRENT_LIST_DIR}/../src "$<TARGET_PROPERTY:pal_ll_win32,INTERFACE_INCLUDE_DIRECTORIES>")
target_link_libraries(win32_ll_reals pal_ll_interfaces_reals synchronization)
