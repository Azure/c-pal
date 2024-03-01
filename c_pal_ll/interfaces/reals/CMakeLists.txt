#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(reals_pal_ll_h_files
    real_interlocked_renames.h
    real_interlocked_undo_rename.h
    real_sync.h
    real_sync_renames.h
    real_threadapi.h
    real_threadapi_renames.h
)

add_library(pal_ll_interfaces_reals INTERFACE)
target_include_directories(pal_ll_interfaces_reals INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pal_ll_interfaces_reals INTERFACE c_logging_v2)
