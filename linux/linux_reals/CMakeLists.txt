#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(linux_reals_c_files
    real_threadapi_linux.c
    real_interlocked.c
    real_gballoc_ll_${gballoc_ll_type_lower}.c
    real_gballoc_hl_${gballoc_hl_type_lower}.c
    real_pipe.c
    real_sync.c
)

set(linux_reals_h_files
    real_interlocked_renames.h
    real_interlocked.h
    ../../common/reals/real_refcount.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(linux_reals ${linux_reals_c_files} ${linux_reals_h_files})
target_include_directories(linux_reals PUBLIC . ${CMAKE_CURRENT_LIST_DIR}/../../common/reals)
target_link_libraries(linux_reals pal_interfaces pal_interfaces_reals rt uuid pthread)
