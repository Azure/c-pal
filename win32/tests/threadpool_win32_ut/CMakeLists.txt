#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName threadpool_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
threadpool_win32_mocked.c
../../src/string_utils.c
)

set(${theseTestsName}_h_files
../../../interfaces/inc/c_pal/threadpool.h
../../inc/c_pal/string_utils.h
../mocked/inc/mock_threadpool_timer.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces synchronization c_pal_reals pal_ll_interfaces
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/threadpool_win32_ut_pch.h"
)

target_include_directories(${theseTestsName}_lib_${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../mocked/inc)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()