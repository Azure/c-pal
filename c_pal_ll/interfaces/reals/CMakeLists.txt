#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(reals_pal_ll_h_files
    real_sync.h
    real_sync_renames.h
)

add_library(pal_ll_interfaces_reals INTERFACE)
target_include_directories(pal_ll_interfaces_reals INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pal_ll_interfaces_reals INTERFACE c_logging)
