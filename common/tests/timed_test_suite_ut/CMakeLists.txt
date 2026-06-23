#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName timed_test_suite_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    main.c
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS ${PAL_OS} c_logging_v2)
