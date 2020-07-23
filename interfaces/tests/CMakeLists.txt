#Copyright (C) Microsoft Corporation. All rights reserved.

add_subdirectory(reals)

if(${run_ut_tests})
    add_subdirectory(lock_ut)
endif()

if(${run_int_tests})
    add_subdirectory(interlocked_int)
    if(MSVC)
        # waiting for timer to be implemented on linux
        add_subdirectory(sync_int)
        add_subdirectory(file_int)
    endif()
endif()

if(${run_perf_tests} AND WIN32)
    build_test_folder(gballoc_hl_perf)
endif()
