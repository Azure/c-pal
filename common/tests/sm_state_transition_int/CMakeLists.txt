#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName sm_state_transition_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    hl.c
    hl_2.c
    ll.c
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
    hl.h
    hl_2.h
    interface.h
    ll.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal" ADDITIONAL_LIBS pal_interfaces c_pal)
