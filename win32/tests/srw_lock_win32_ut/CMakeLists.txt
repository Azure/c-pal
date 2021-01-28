#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName srw_lock_win32_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    srw_lock_mocked.c
    ../../src/string_utils.c
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals)

