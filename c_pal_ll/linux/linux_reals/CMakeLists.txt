#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(linux_ll_reals_c_files
    real_threadapi_linux.c
    real_interlocked.c
    real_sync.c
)

set(linux_ll_reals_h_files
    real_interlocked.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../src)

add_library(linux_ll_reals ${linux_ll_reals_c_files} ${linux_ll_reals_h_files})
set_target_properties(linux_ll_reals PROPERTIES LINKER_LANGUAGE C)
target_include_directories(linux_ll_reals PUBLIC . ${CMAKE_CURRENT_LIST_DIR}/../../../common/reals  ${CMAKE_CURRENT_LIST_DIR}/../../interfaces/inc)
target_link_libraries(linux_ll_reals pal_ll_interfaces_reals rt uuid pthread umock_c)
