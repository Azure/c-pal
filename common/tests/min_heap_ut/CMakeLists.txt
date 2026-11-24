#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName min_heap_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/min_heap.c
)

set(${theseTestsName}_h_files
    ../../inc/c_pal/min_heap.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/min_heap_ut_pch.h"
)
