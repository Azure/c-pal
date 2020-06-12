#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(reals_linux_c_files
    real_threadapi_linux.c
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(reals_linux ${reals_linux_c_files})
target_link_libraries(reals_linux reals_pal_interface)
