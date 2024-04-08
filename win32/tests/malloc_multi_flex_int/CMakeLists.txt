#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName malloc_multi_flex_int) 

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    test_module.c
)

set(${theseTestsName}_h_files
    test_module.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS pal_interfaces) 
