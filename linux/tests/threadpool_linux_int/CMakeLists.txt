#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName threadpool_linux_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_interfaces pal_linux pal_ll_interfaces pal_ll_linux)
