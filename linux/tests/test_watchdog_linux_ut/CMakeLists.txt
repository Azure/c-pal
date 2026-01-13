#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName test_watchdog_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    test_watchdog_linux_mocked.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/test_watchdog.h
    ../mocked/inc/mock_timer.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_interfaces c_pal_reals pal_ll_interfaces
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/test_watchdog_linux_ut_pch.h"
)

target_include_directories(${theseTestsName}_lib_${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../mocked/inc)
