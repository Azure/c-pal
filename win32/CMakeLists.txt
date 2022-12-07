#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/c_pal/call_once.h
    ../common/inc/c_pal/lazy_init.h
    ../common/inc/c_pal/s_list.h
)

set(pal_common_c_files
    ../common/src/call_once.c
    ../common/src/lazy_init.c
    ../common/src/s_list.c
)

#determining which one of the GBALLOC_LL implementations to use. By convention the file is called "gballoc_ll_" followed by "type".
string(TOLOWER "${GBALLOC_LL_TYPE}" gballoc_ll_type_lower)
set(gballoc_ll_c gballoc_ll_${gballoc_ll_type_lower}.c)

#determining which one of the GBALLOC_HL implementations to use. By convention the file is called "gballoc_hl_" followed by "type".
string(TOLOWER "${GBALLOC_HL_TYPE}" gballoc_hl_type_lower)
set(gballoc_hl_c gballoc_hl_${gballoc_hl_type_lower}.c)

set(pal_win32_h_files
    ${pal_common_h_files}
    inc/c_pal/execution_engine_win32.h
)

set(pal_win32_c_files
    ${pal_common_c_files}
    src/arithmetic_win32.c
    src/execution_engine_win32.c
    src/async_socket_win32.c
    src/threadpool_win32.c
    src/interlocked_win32.c
    src/pipe_win32.c
    src/platform_win32.c
    src/threadapi_win32.c
    src/srw_lock_win32.c
    src/string_utils.c
    src/timer_win32.c
    src/sync_win32.c
    src/sysinfo_win32.c
    src/file_win32.c
    src/uuid_win32.c
    src/${gballoc_ll_c}
    src/${gballoc_hl_c}
)

FILE(GLOB pal_win32_md_files "devdoc/*.md")
FILE(GLOB pal_common_md_files "../common/devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_win32_md_files} ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)

add_library(pal_win32 ${pal_win32_h_files} ${pal_win32_c_files} ${pal_win32_md_files} ${pal_common_md_files})

target_link_libraries(pal_win32 pal_interfaces ws2_32 synchronization rpcrt4 c_logging)
if(${GBALLOC_LL_TYPE} STREQUAL "MIMALLOC")
    target_link_libraries(pal_win32 mimalloc-obj)
endif()

if(${GBALLOC_LL_TYPE} STREQUAL "JEMALLOC")
    target_link_libraries(pal_win32 jemalloc)
endif()

target_include_directories(pal_win32 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc ${CMAKE_CURRENT_LIST_DIR}/src)


add_subdirectory(reals)
add_subdirectory(tests)
add_subdirectory(../common/tests common/tests)
