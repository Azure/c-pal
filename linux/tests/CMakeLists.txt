#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    build_test_folder(async_socket_linux_ut)
    build_test_folder(completion_port_linux_ut)
    build_test_folder(error_handling_linux_ut)
    build_test_folder(execution_engine_linux_ut)
    build_test_folder(file_util_linux_ut)
    build_test_folder(gballoc_ll_passthrough_ut)
    build_test_folder(gballoc_hl_passthrough_ut)
    build_test_folder(linux_reals_ut)
    build_test_folder(pipe_linux_ut)
    build_test_folder(platform_linux_ut)
    build_test_folder(srw_lock_linux_ut)
    build_test_folder(srw_lock_ll_linux_ut)
    build_test_folder(socket_transport_linux_ut)
    build_test_folder(sysinfo_linux_ut)
    build_test_folder(timer_linux_ut)
    build_test_folder(threadpool_linux_ut)
    build_test_folder(uuid_linux_ut)
endif()

if(${run_int_tests})
    build_test_folder(async_socket_linux_int)
    build_test_folder(gballoc_ll_passthrough_int)
    build_test_folder(string_utils_int)
endif()
