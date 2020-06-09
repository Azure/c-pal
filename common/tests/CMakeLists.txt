#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    add_subdirectory(gballoc_ut)
    add_subdirectory(gballoc_without_init_ut)
endif()

if(${run_int_tests})
endif()
