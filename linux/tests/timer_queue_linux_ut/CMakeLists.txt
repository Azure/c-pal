#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName timer_queue_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/timer_queue_linux.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/timer_queue_linux.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/timer_queue_linux_ut_pch.h"
)
