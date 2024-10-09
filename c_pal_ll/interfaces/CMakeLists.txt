#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_interfaces_h_files
    inc/c_pal/interlocked.h
    inc/c_pal/interlocked_macros.h
    inc/c_pal/sync.h
    inc/c_pal/threadapi.h
)

FILE(GLOB pal_ll_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_interfaces_md_files})

add_library(pal_ll_interfaces INTERFACE)
target_include_directories(pal_ll_interfaces INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${MACRO_UTILS_INC_FOLDER})

target_sources(pal_ll_interfaces INTERFACE ${pal_ll_interfaces_h_files})

add_mock_library(pal_ll_interfaces HEADER_FILES ${pal_ll_interfaces_h_files})

add_subdirectory(reals)

add_subdirectory(tests)
