#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName tqueue_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
tqueue_foo.c
)

set(${theseTestsName}_h_files
../../inc/c_pal/tqueue.h
tqueue_foo.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal)
