#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName threadpool_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    mock_threadpool.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/threadpool.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal)
