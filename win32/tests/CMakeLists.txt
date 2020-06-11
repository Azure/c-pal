#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    add_subdirectory(execution_engine_win32_ut)
    add_subdirectory(async_socket_win32_ut)
    add_subdirectory(threadpool_win32_ut)
    add_subdirectory(interlocked_win32_ut)
    add_subdirectory(lock_win32_ut)
    add_subdirectory(interlocked_hl_win32_ut)
    add_subdirectory(platform_win32_ut)
    add_subdirectory(uniqueid_win32_ut)
    add_subdirectory(timer_win32_ut)
    add_subdirectory(srw_lock_win32_ut)
    add_subdirectory(reals_ut)
endif()

if(${run_int_tests})
    add_subdirectory(async_socket_win32_int)
    add_subdirectory(threadpool_win32_int)
    add_subdirectory(string_utils_int)
endif()
