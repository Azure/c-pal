#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName process_watchdog_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
    ${CMAKE_CURRENT_LIST_DIR}/../../../interfaces/tests/process_watchdog_int_common/process_watchdog_int_common.h
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../interfaces/tests/process_watchdog_int_common)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS c_pal c_logging_v2)

# Ensure child executable is built before the test
add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} process_watchdog_int_child)
