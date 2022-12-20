#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName async_socket_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    async_socket_linux_mocked.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/async_socket.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_interfaces c_pal_reals)
