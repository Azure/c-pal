#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName srw_lock_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    mock_srw_lock.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/srw_lock.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/srw_lock_linux_ut_pch.h"
)