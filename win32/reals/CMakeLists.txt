#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(reals_win32_c_files
    real_arithmetic.c
    real_pipe.c
    real_srw_lock_win32.c
    real_socket_transport_win32.c
    real_srw_lock_ll_win32.c
    real_string_utils_win32.c
    real_timer_win32.c
    real_uuid.c
    real_gballoc_ll_${gballoc_ll_type_lower}.c
    real_gballoc_hl_${gballoc_hl_type_lower}.c
    real_async_socket.c
    real_threadpool.c
    real_execution_engine.c #note: also contains the code for execution_engine_win32.c
    real_execution_engine_win32.c #note:empty file
    ../../common/reals/real_call_once.c
    ../../common/reals/real_ps_util.c
    ../../common/reals/real_sm.c
    ../../common/reals/real_s_list.c
    ../../common/reals/real_interlocked_hl.c
    ../../common/reals/real_thandle_log_context_handle.c
    ../../common/reals/real_threadpool_thandle.c
    ../../common/reals/real_threadpool_timer_thandle.c
    ../../common/reals/real_threadpool_work_item_thandle.c
    real_job_object_helper.c
)

set(reals_win32_h_files
    real_arithmetic.h
    real_arithmetic_renames.h
    real_uuid.h
    real_uuid_renames.h
    ../../common/reals/real_refcount.h
    real_async_socket.h
    real_async_socket_renames.h
    real_threadpool.h
    real_threadpool_renames.h
    real_execution_engine.h
    real_execution_engine_renames.h
    real_execution_engine_win32.h
    real_execution_engine_win32_renames.h
    ../../common/reals/real_call_once.h
    ../../common/reals/real_call_once_renames.h
    ../../common/reals/real_ps_util.h
    ../../common/reals/real_ps_util_renames.h
    ../../common/reals/real_lazy_init.h
    ../../common/reals/real_lazy_init_renames.h
    ../../common/reals/real_s_list.h
    ../../common/reals/real_s_list_renames.h
    ../../common/reals/real_sm.h
    ../../common/reals/real_sm_renames.h
    ../../common/reals/real_interlocked_hl.h
    ../../common/reals/real_interlocked_hl_renames.h
    ../../common/reals/real_thandle_helper.h
    ../../common/reals/real_thandle_log_context_handle.h
    ../../common/reals/real_thandle_log_context_handle_renames.h
    ../../common/reals/real_threadpool_thandle.h
    ../../common/reals/real_threadpool_timer_thandle.h
    ../../common/reals/real_threadpool_work_item_thandle.h
    real_job_object_helper.h
    real_job_object_helper_renames.h
)

add_library(win32_reals ${reals_win32_c_files} ${reals_win32_h_files})
target_include_directories(win32_reals PUBLIC . ${CMAKE_CURRENT_LIST_DIR}/../../common/reals ${CMAKE_CURRENT_LIST_DIR}/../src "$<TARGET_PROPERTY:pal_win32,INTERFACE_INCLUDE_DIRECTORIES>")
target_link_libraries(win32_reals pal_interfaces_reals win32_ll_reals synchronization rpcrt4)

if(${GBALLOC_LL_TYPE} STREQUAL "MIMALLOC")
    target_link_libraries(win32_reals mimalloc-obj)
endif()
if(${GBALLOC_LL_TYPE} STREQUAL "JEMALLOC")
    target_link_libraries(win32_reals jemalloc)
endif()
