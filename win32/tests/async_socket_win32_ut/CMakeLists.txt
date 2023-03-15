#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_socket_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
async_socket_win32_mocked.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/c_pal/async_socket.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces synchronization ws2_32 c_pal_reals pal_ll_interfaces c_pal_ll_reals)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()