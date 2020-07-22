#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName gballoc_win32_heap_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
./gballoc_win32_heap_mocked.c
)

set(${theseTestsName}_h_files
../../inc/gballoc_win32_heap.h
)

build_test_artifacts(${theseTestsName} ON "tests/custom_heap" ADDITIONAL_LIBS azure_c_util)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
endif()