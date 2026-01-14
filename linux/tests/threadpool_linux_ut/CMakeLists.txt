#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName threadpool_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    mock_threadpool.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/threadpool.h
    ../mocked/inc/mock_timer.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/threadpool_linux_ut_pch.h"
)

target_include_directories(${theseTestsName}_lib_${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../mocked/inc)