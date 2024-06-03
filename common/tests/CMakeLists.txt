#Copyright (C) Microsoft Corporation. All rights reserved.

if(MSVC)
    set(PAL_OS pal_win32)
else()
    set(PAL_OS pal_linux)
endif()

# unit tests
if(${run_unittests})
    build_test_folder(interlocked_hl_ut)
    build_test_folder(log_critical_and_terminate_ut)
    build_test_folder(ps_util_ut)
    build_test_folder(refcount_ut)
    build_test_folder(call_once_ut)
    build_test_folder(lazy_init_ut)
    build_test_folder(real_thandle_helper_ut)
    build_test_folder(sm_ut)
    build_test_folder(s_list_ut)
    build_test_folder(thandle_2_ut)
    build_test_folder(thandle_ut)
    build_test_folder(thandle_ptr_ut)
    build_test_folder(real_thandle_log_context_handle_ut)
endif()

if(${run_int_tests})
    build_test_folder(arithmetic_int)
    build_test_folder(async_socket_int)
    build_test_folder(call_once_int)
    build_test_folder(interlocked_hl_int)
    build_test_folder(lazy_init_int)
    build_test_folder(sm_int)
    build_test_folder(thandle_ptr_int)
endif()


