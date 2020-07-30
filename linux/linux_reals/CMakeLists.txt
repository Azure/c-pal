#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(linux_reals_c_files
    real_threadapi_linux.c
    real_interlocked.c
)

set(linux_reals_h_files
    real_interlocked_renames.h
    real_interlocked.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(linux_reals ${linux_reals_c_files} ${linux_reals_h_files})
target_include_directories(linux_reals PUBLIC .)
target_link_libraries(linux_reals pal_interfaces pal_interfaces_reals)