#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName srw_lock_ll_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    srw_lock_ll_win32_mocked.c
    ../../src/string_utils.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals pal_ll_interfaces c_pal_ll_reals pal_interfaces_reals)

