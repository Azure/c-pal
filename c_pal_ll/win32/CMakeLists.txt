#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_win32_h_files
    ../interfaces/inc/c_pal/interlocked.h
    ../interfaces/inc/c_pal/sync.h
    ../interfaces/inc/c_pal/threadapi.h
)

set(pal_ll_win32_c_files    
    src/interlocked_win32.c
    src/sync_win32.c
    src/threadapi_win32.c
)

FILE(GLOB pal_ll_win32_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_win32_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)
include_directories(../c_pal_ll/interfaces/inc)

add_library(pal_ll_win32 ${pal_ll_win32_h_files} ${pal_ll_win32_c_files} ${pal_ll_win32_md_files})

set_target_properties(pal_ll_win32 PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(pal_ll_win32 pal_ll_interfaces synchronization rpcrt4 c_logging)

target_include_directories(pal_ll_win32 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc ${CMAKE_CURRENT_LIST_DIR}/src)

add_subdirectory(reals)
add_subdirectory(tests)
