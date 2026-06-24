#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName gballoc_ll_mimalloc_int) 

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/gballoc_ll_mimalloc.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS mimalloc-obj pal_interfaces) 