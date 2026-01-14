#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName process_watchdog_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    process_watchdog_win32_mocked.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/process_watchdog.h
    ../mocked/inc/mock_threadpool_timer.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces pal_ll_interfaces c_pal_reals c_pal_ll_reals ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/process_watchdog_win32_ut_pch.h")
