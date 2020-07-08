#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_win32_h_files
    ${pal_common_h_files}
    inc/execution_engine_win32.h
    inc/refcount_os.h
)

set(pal_win32_c_files
    ${pal_common_c_files}
    src/execution_engine_win32.c
    src/async_socket_win32.c
    src/threadpool_win32.c
    src/lock_win32.c
    src/interlocked_win32.c
    src/interlocked_hl_win32.c
    src/platform_win32.c
    src/threadapi_win32.c
    src/uniqueid_win32.c
    src/srw_lock_win32.c
    src/string_utils.c
    src/timer.c
    src/sync_win32.c
    src/file_win32.c
)

FILE(GLOB pal_win32_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_win32_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)

add_library(pal_win32 ${pal_win32_h_files} ${pal_win32_c_files} ${pal_win32_md_files})
target_link_libraries(pal_win32 pal_interfaces pal_internal ws2_32 synchronization rpcrt4 azure_c_logging)
target_include_directories(pal_win32 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

add_subdirectory(tests)
add_subdirectory(../common/tests common/tests)