#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName worker_pool_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    mock_worker_pool_linux.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/worker_pool_linux.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/worker_pool_linux_ut_pch.h"
)
