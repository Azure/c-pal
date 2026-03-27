#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName threadpool_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS real_process_watchdog c_pal_ll_reals pal_interfaces c_pal)
