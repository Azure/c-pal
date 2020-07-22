#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName gballoc_hl_metrics_wout_init_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
./gballoc_hl_metrics_mocked.c
)

set(${theseTestsName}_h_files

)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_pal/common" ADDITIONAL_LIBS azure_c_pal)

if("${building}" STREQUAL "exe")
    set_target_properties(${theseTestsName}_exe PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
    set_target_properties(${theseTestsName}_dll PROPERTIES LINK_FLAGS "/ignore:4217")
endif()
