#Copyright (C) Microsoft Corporation. All rights reserved.

add_library(pal_internal INTERFACE)
target_link_libraries(pal_internal INTERFACE pal_interfaces)
target_include_directories(pal_internal INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)
