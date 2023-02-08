#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName lazy_init_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/lazy_init.c
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals pal_ll_interfaces c_pal_ll_reals)

