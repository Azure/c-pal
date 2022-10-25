#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName srw_lock_win32_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/int" ADDITIONAL_LIBS c_pal)

