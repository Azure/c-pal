#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName srw_lock_ll_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    srw_lock_ll_linux_mocked.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_interfaces c_pal c_pal_reals pal_ll_interfaces c_pal_ll_reals pal_interfaces_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/srw_lock_ll_linux_ut_pch.h"
)
