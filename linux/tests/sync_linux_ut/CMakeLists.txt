#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName sync_linux_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
mock_sync.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/azure_c_pal/sync.h
mock_sync.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/linux" ADDITIONAL_LIBS pal_interfaces)
