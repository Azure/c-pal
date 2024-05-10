#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName thandle_ptr_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    example.c
    example_incomplete_type.c
)

set(${theseTestsName}_h_files
    example.h
    example_incomplete_type.h
    ../../inc/c_pal/thandle_ptr.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal c_pal_reals)

