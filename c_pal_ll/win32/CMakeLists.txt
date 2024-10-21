#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_win32_c_files
    src/interlocked_win32.c
    src/sync_win32.c
    src/threadapi_win32.c
)

FILE(GLOB pal_ll_win32_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_win32_md_files})

add_library(pal_ll_win32 ${pal_ll_win32_c_files} ${pal_ll_win32_md_files})
set_target_properties(pal_ll_win32 PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(pal_ll_win32 pal_ll_interfaces synchronization rpcrt4 c_logging_v2 umock_c)

target_include_directories(pal_ll_win32 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/src
    ../../common/inc
    ../c_pal_ll/interfaces/inc
)

add_subdirectory(reals)
add_subdirectory(tests)
