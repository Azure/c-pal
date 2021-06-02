#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName gballoc_hl_perf)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_pal/common" ADDITIONAL_LIBS c_pal)
