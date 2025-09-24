#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName interlocked_hl_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/interlocked_hl.c
)

set(${theseTestsName}_h_files
../../inc/c_pal/interlocked_hl.h
)

include_directories(BEFORE SYSTEM ../mocked/inc)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/interlocked_hl_ut_pch.h"
)