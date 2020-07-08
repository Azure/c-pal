#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName execution_engine_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
execution_engine_win32_mocked.c
)

set(${theseTestsName}_h_files
../../inc/execution_engine_win32.h
../../../interfaces/inc/execution_engine.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS pal_interfaces)

set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
if(use_cppunittest)
    set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
endif()
