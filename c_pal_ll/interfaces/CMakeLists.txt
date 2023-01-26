#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_interfaces_h_files
    inc/c_pal/interlocked.h
    inc/c_pal/sync.h
)

FILE(GLOB pal_ll_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_interfaces_md_files})

set(PAL_LL_INTERFACES_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using the PAL interfaces" FORCE)

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(${MACRO_UTILS_INC_FOLDER})

add_library(pal_ll_interfaces INTERFACE)
target_include_directories(pal_ll_interfaces INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)

add_subdirectory(reals)

add_subdirectory(tests)