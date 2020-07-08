#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName uniqueid_ut)

set(uniqueid_ut_test_file ${theseTestsName}_linux.c)

set(${theseTestsName}_test_files
    ${uniqueid_ut_test_file}
)


set(${theseTestsName}_c_files
    ../../src/uniqueid_linux.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/azure_c_pal/uniqueid.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal" ADDITIONAL_LIBS pal_interfaces uuid)

