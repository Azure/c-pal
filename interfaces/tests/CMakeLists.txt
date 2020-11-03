#Copyright (C) Microsoft Corporation. All rights reserved.

if(${run_int_tests})
    build_test_folder(interlocked_int)
    build_test_folder(timer_int)
    build_test_folder(sync_int)
    build_test_folder(sysinfo_int)
    if(MSVC)
        # waiting for timer to be implemented on linux
        build_test_folder(file_int)
    endif()
endif()

if(${run_perf_tests} AND WIN32)
    build_test_folder(gballoc_hl_perf)
endif()
