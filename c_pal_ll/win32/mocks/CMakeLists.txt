#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(mocks_win32_ll_c_files
    mocks_win32_ll.c
)

set(mocks_win32_ll_h_files
    mocks_win32_ll.h
)

set_source_files_properties(mocks_win32_ll.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

add_library(win32_ll_mocks STATIC ${mocks_win32_ll_h_files} ${mocks_win32_ll_c_files})
target_include_directories(win32_ll_mocks PUBLIC $<TARGET_PROPERTY:pal_ll_win32,INTERFACE_INCLUDE_DIRECTORIES>)
target_precompile_headers(win32_ll_mocks PUBLIC mocks_win32_ll.h)
