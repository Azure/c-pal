#Copyright (c) Microsoft. All rights reserved.

compileAsC11()
set(theseTestsName execution_engine_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    execution_engine_win32_mocked.c
    ../../src/interlocked_win32.c
)

set(${theseTestsName}_h_files
../../inc/c_pal/execution_engine_win32.h
../../../interfaces/inc/c_pal/execution_engine.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_pal/win32" ADDITIONAL_LIBS pal_interfaces c_pal_reals)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

