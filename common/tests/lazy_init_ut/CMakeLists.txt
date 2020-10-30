#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName lazy_init_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/lazy_init.c
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals)

