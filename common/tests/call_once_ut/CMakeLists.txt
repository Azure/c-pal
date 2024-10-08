#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName call_once_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/call_once.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals pal_ll_interfaces win32_ll_mocks)

