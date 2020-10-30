#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName interlocked_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
mock_interlocked.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/c_pal/interlocked.h
mock_interlocked.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces)
