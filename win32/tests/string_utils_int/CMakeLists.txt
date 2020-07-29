#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName string_utils_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_cpp_files
    nothing.cpp
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_pal/win32" ADDITIONAL_LIBS pal_interfaces pal_win32)
