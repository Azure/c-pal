#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName srw_lock_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    srw_lock_mocked.c
    ../../src/string_utils.c
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS pal_interfaces pal_win32)

