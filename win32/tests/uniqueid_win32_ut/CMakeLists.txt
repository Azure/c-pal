#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName uniqueid_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

if(WIN32)
set(${theseTestsName}_c_files
    uniqueid_win32_mocked.c
)
else()
set(${theseTestsName}_c_files
    ../../src/uniqueid_win32.c
)
endif()

set(${theseTestsName}_h_files
    ../../../interfaces/inc/uniqueid.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS rpcrt4.lib pal_interfaces)

