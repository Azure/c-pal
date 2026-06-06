#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName srw_lock_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/int" ADDITIONAL_LIBS c_pal)

