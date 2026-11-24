#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName execution_engine_linux_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS pal_interfaces c_pal)
