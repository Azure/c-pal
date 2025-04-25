#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/c_pal/call_once.h
    ../common/inc/c_pal/interlocked_hl.h
    ../common/inc/c_pal/lazy_init.h
    ../common/inc/c_pal/log_critical_and_terminate.h
    ../common/inc/c_pal/ps_util.h
    ../common/inc/c_pal/s_list.h
    ../common/inc/c_pal/sm.h
    ../common/inc/c_pal/thandle_ll.h
    ../common/inc/c_pal/thandle.h
    ../common/inc/c_pal/thandle_ptr.h
    ../common/inc/c_pal/thandle_log_context_handle.h
    ../common/inc/c_pal/tqueue.h
    ../common/inc/c_pal/tqueue_ll.h
)

set(pal_common_c_files
    ../common/src/call_once.c
    ../common/src/interlocked_hl.c
    ../common/src/lazy_init.c
    ../common/src/ps_util.c
    ../common/src/s_list.c
    ../common/src/sm.c
    ../common/src/thandle_log_context_handle.c
    ../common/src/tqueue.c
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
    inc/c_pal/job_object_helper.h
)

set(pal_win32_c_files
    ${pal_common_c_files}
    src/arithmetic_win32.c
    src/error_handling_win32.c
    src/execution_engine_win32.c
    src/async_socket_win32.c
    src/threadpool_win32.c
    src/pipe_win32.c
    src/platform_win32.c
    src/single_performance_counter_win32.c
    src/socket_transport_win32.c
    src/srw_lock_win32.c
    src/srw_lock_ll_win32.c
    src/string_utils.c
    src/timer_win32.c
    src/sysinfo_win32.c
    src/file_win32.c
    src/uuid_win32.c
    src/${gballoc_ll_c}
    src/${gballoc_hl_c}
    src/job_object_helper.c
)

FILE(GLOB pal_win32_md_files "devdoc/*.md")
FILE(GLOB pal_common_md_files "../common/devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_win32_md_files} ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)
include_directories(../c_pal_ll/interfaces/inc)

add_library(pal_win32 ${pal_win32_h_files} ${pal_win32_c_files} ${pal_win32_md_files} ${pal_common_md_files})

target_link_libraries(pal_win32 pal_interfaces c_logging_v2 umock_c ws2_32 synchronization rpcrt4 )
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
