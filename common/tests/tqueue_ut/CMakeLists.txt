#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName tqueue_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
../../inc/c_pal/tqueue.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal c_pal_reals)
