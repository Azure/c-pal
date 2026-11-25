#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Separate from c_pal because it replaces the global operator new/delete in every binary that links it.
add_library(c_pal_mocked_new_delete STATIC
    inc/c_pal_test_helpers/mocked_new_delete.h
    src/mocked_new_delete.cpp
    devdoc/mocked_new_delete_requirements.md
)
target_include_directories(c_pal_mocked_new_delete PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_link_libraries(c_pal_mocked_new_delete PUBLIC c_pal)

if(${run_unittests})
    build_test_folder(tests/mocked_new_delete_ut)
endif()
