#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName lock_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/lock_win32.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/lock.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS pal_interfaces)
