#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/c_pal/call_once.h
    ../common/inc/c_pal/containing_record.h
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
    ../common/src/lazy_init.c
    ../common/src/interlocked_hl.c
    ../common/src/ps_util.c
    ../common/src/s_list.c
    ../common/src/sm.c
    ../common/src/thandle_log_context_handle.c
    ../common/src/tqueue.c
    ../common/src/nil_uuid.c
)

if(${run_helgrind})
    add_definitions(-DUSE_HELGRIND)
endif()


#determining which one of the GBALLOC_LL implementations to use. By convention the file is called "gballoc_ll_" followed by "type".
string(TOLOWER "${GBALLOC_LL_TYPE}" gballoc_ll_type_lower)
set(gballoc_ll_c gballoc_ll_${gballoc_ll_type_lower}.c)

#determining which one of the GBALLOC_HL implementations to use. By convention the file is called "gballoc_hl_" followed by "type".
string(TOLOWER "${GBALLOC_HL_TYPE}" gballoc_hl_type_lower)
set(gballoc_hl_c gballoc_hl_${gballoc_hl_type_lower}.c)

set(pal_linux_h_files
    ${pal_common_h_files}
    inc/c_pal/completion_port_linux.h
    inc/c_pal/execution_engine_linux.h
    inc/c_pal/platform_linux.h
    inc/c_pal/windows_defines.h
    inc/c_pal/tqueue_threadpool_work_item.h
)

set(pal_linux_c_files
    ${pal_common_c_files}
    src/arithmetic_linux.c
    src/async_socket_linux.c
    src/completion_port_linux.c
    src/error_handling_linux.c
    src/execution_engine_linux.c
    src/file_linux.c
    src/file_util_linux.c
    src/pipe_linux.c
    src/platform_linux.c
    src/single_performance_counter_linux.c
    src/socket_transport_linux.c
    src/srw_lock_linux.c
    src/srw_lock_ll_linux.c
    src/string_utils.c
    src/string_utils.c
    src/sysinfo_linux.c
    src/test_watchdog_linux.c
    src/threadpool_linux.c
    src/tqueue_threadpool_work_item.c
    src/timer_linux.c
    src/uuid_linux.c
    src/${gballoc_ll_c}
    src/${gballoc_hl_c}
)

FILE(GLOB pal_linux_md_files "devdoc/*.md")
FILE(GLOB pal_common_md_files "../common/devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_linux_md_files} ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)
include_directories(../c_pal_ll/interfaces/inc)

add_library(pal_linux ${pal_linux_h_files} ${pal_linux_c_files} ${pal_linux_md_files} ${pal_common_md_files})
target_link_libraries(pal_linux pal_ll_linux pal_interfaces rt uuid pthread)
target_include_directories(pal_linux PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

# make an install target so we can produce a Linux native client package.
install_library_with_prefix(pal_linux c_pal  ${pal_linux_h_files} )

add_subdirectory(linux_reals)
add_subdirectory(tests)
add_subdirectory(../common/tests common/tests)
