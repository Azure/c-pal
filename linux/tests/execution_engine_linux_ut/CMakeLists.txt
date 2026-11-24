#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName execution_engine_linux_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    mock_execution_engine.c
)

set(${theseTestsName}_h_files
    ../../../interfaces/inc/c_pal/execution_engine.h
    ../../inc/c_pal/execution_engine_linux.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/linux" ADDITIONAL_LIBS c_pal_reals c_pal 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/execution_engine_linux_ut_pch.h"
)
