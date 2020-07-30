#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName async_socket_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
async_socket_win32_mocked.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/azure_c_pal/async_socket.h
)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS pal_interfaces synchronization ws2_32)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
endif()