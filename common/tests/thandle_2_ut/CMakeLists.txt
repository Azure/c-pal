#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName thandle_2_ut) #these tests are crafted to only prove the usage of different patterns for memory allocation (global, at type define time, at instantiation time)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    t_off.c
    t_on.c
    two_thandles_same_file.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/thandle.h
    ../../inc/c_pal/thandle_ll.h
    malloc_mocks.h
    t_off.h
    t_on.h
    two_thandles_same_file.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal c_pal_reals)
