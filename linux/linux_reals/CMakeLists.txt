#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(linux_reals_c_files
    real_async_socket.c
    real_execution_engine.c
    real_execution_engine_linux.c #note:empty file
    real_gballoc_ll_${gballoc_ll_type_lower}.c
    real_gballoc_hl_${gballoc_hl_type_lower}.c
    real_pipe.c
    real_socket_transport_linux.c
    real_string_utils_linux.c
    real_srw_lock.c
    real_srw_lock_ll_linux.c
    real_threadpool.c
    real_timer.c
    real_uuid.c
    real_tqueue_threadpool_work_item.c
    ../../common/reals/real_call_once.c
    ../../common/reals/real_s_list.c
    ../../common/reals/real_ps_util.c
    ../../common/reals/real_lazy_init.c
    ../../common/reals/real_sm.c
    ../../common/reals/real_interlocked_hl.c
    ../../common/reals/real_thandle_log_context_handle.c
    ../../common/reals/real_threadpool_thandle.c
    ../../common/reals/real_threadpool_timer_thandle.c
    ../../common/reals/real_threadpool_work_item_thandle.c
)

set(linux_reals_h_files
    real_async_socket.h
    real_async_socket_renames.h
    real_execution_engine.h
    real_execution_engine_renames.h
    real_execution_engine_linux.h
    real_execution_engine_linux_renames.h
    real_srw_lock.h
    real_srw_lock_renames.h
    real_threadpool.h
    real_threadpool_renames.h
    real_uuid_renames.h
    real_uuid.h
    real_tqueue_threadpool_work_item.h
    real_tqueue_threadpool_work_item_renames.h
    ../../common/reals/real_refcount.h
    ../../common/reals/real_s_list.h
    ../../common/reals/real_s_list_renames.h
    ../../common/reals/real_ps_util.h
    ../../common/reals/real_ps_util_renames.h
    ../../common/reals/real_call_once.h
    ../../common/reals/real_call_once.h
    ../../common/reals/real_lazy_init.h
    ../../common/reals/real_lazy_init_renames.h
    ../../common/reals/real_sm.h
    ../../common/reals/real_sm_renames.h
    ../../common/reals/real_interlocked_hl.h
    ../../common/reals/real_interlocked_hl_renames.h
    ../../common/reals/real_socket_transport.h
    ../../common/reals/real_socket_transport_renames.h
    ../../common/reals/real_thandle_helper.h
    ../../common/reals/real_threadpool_thandle.h
    ../../common/reals/real_threadpool_timer_thandle.h
    ../../common/reals/real_threadpool_work_item_thandle.h
    ../../interfaces/reals/real_timer.h
    ../../interfaces/reals/real_timer_renames.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(linux_reals ${linux_reals_c_files} ${linux_reals_h_files})
target_include_directories(linux_reals PUBLIC . ${CMAKE_CURRENT_LIST_DIR}/../../common/reals ${CMAKE_CURRENT_LIST_DIR}/../../interfaces/reals ${CMAKE_CURRENT_LIST_DIR}/../inc)
target_link_libraries(linux_reals linux_ll_reals pal_interfaces pal_interfaces_reals rt uuid pthread)
