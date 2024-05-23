#Copyright (C) Microsoft Corporation. All rights reserved.

if(${run_int_tests})
    build_test_folder(pipe_int)
    build_test_folder(socket_mgr_int)
    build_test_folder(srw_lock_int)
    build_test_folder(srw_lock_ll_int)
    build_test_folder(sysinfo_int)
    build_test_folder(timer_int)
    build_test_folder(uuid_int)
    if(MSVC)
        # file_int is not yet there on Linux
        build_test_folder(file_int)
    endif()
endif()

if(${run_perf_tests} AND WIN32)
    build_test_folder(gballoc_hl_perf)
endif()
