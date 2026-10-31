#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# unique_thandle_compile_checks() checks at configure time that the compiler accepts the documented use of UNIQUE_THANDLE (compiles.c)
# and rejects each case of rejects_with_errors.c and rejects_with_warnings_as_errors.c with the expected diagnostic.
# Each case starts with #line 1000 "<case>", so a diagnostic names the case it belongs to.
# The checks compile with CMAKE_C_FLAGS, which treat warnings as errors (/WX or -Werror) in c-pal builds.

# checks that the output of a compilation that had to fail has the expected diagnostic for each case
# cases is a list of "<case>|<MSVC diagnostic code>|<gcc diagnostic text>"
function(unique_thandle_compile_checks_expect_cases source result output cases)
    if(${result})
        message(FATAL_ERROR "UNIQUE_THANDLE compile checks: ${source}.c compiles, but each of its cases has to fail to compile:\n${output}")
    endif()

    set(missing_cases)
    foreach(case_entry ${cases})
        string(REPLACE "|" ";" case_fields "${case_entry}")
        list(GET case_fields 0 case_name)
        list(GET case_fields 1 msvc_code)
        list(GET case_fields 2 gcc_text)

        if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
            set(expected_diagnostic "${case_name}\\([0-9]+(,[0-9]+)?\\): (error|warning) ${msvc_code}")
            set(expected_text "${msvc_code}")
        elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            set(expected_diagnostic "${case_name}:[0-9]+:[0-9]+: error: [^\n]*${gcc_text}")
            set(expected_text "an error with \"${gcc_text}\"")
        else()
            set(expected_diagnostic "${case_name}[(:][0-9]+[^\n]*(error|warning)")
            set(expected_text "an error")
        endif()

        if(NOT "${output}" MATCHES "${expected_diagnostic}")
            list(APPEND missing_cases "${case_name}: expected ${expected_text}")
        endif()
    endforeach()

    if(missing_cases)
        string(REPLACE ";" "\n    " missing_cases_text "${missing_cases}")
        message(FATAL_ERROR "UNIQUE_THANDLE compile checks: these cases of ${source}.c lack their expected diagnostic:\n    ${missing_cases_text}\n${output}")
    endif()
endfunction()

function(unique_thandle_compile_checks)
    set(checks_dir ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

    set(error_cases
        "copy_initialization_from_a_parameter|C2075|invalid initializer"
        "copy_initialization_from_a_variable|C2075|invalid initializer"
        "assignment_of_a_handle_in_an_array|C2106|assignment to expression with array type"
        "assignment_through_a_handle_parameter|C2166|assignment of read-only location"
        "assignment_of_the_element_of_a_handle|C2166|assignment of read-only location"
        "assignment_of_the_object_pointer_of_a_handle|C2166|assignment of read-only member"
        "function_returning_a_handle|C2090|function returning an array"
    )

    set(warning_cases
        "move_from_a_const_handle|C4090|discarded-qualifiers"
        "dispose_of_a_const_handle|C4090|discarded-qualifiers"
        "move_from_a_handle_in_a_const_struct|C4090|discarded-qualifiers"
        "copy_of_the_object_pointer_to_a_pointer_to_t|C4133|incompatible-pointer-types"
        "move_from_a_pointer_to_t|C4047|incompatible-pointer-types"
        "move_from_a_handle_of_another_type|C4133|incompatible-pointer-types"
    )

    # the include directories of c_pal and its dependencies are generator expressions, so they are built from the directories of the targets
    set(include_dirs ${checks_dir}/../../inc)
    foreach(target pal_interfaces pal_ll_interfaces macro_utils_c c_logging_v2 umock_c)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
        list(APPEND include_dirs ${target_source_dir}/inc)
    endforeach()

    # definitions that c-pal builds add with add_definitions and target_compile_definitions, which try_compile does not get from CMAKE_C_FLAGS
    set(definitions -DGBALLOC_LL_TYPE_${GBALLOC_LL_TYPE})
    if(MSVC)
        list(APPEND definitions -D_CRT_SECURE_NO_WARNINGS)
    endif()

    # warnings are errors, as in CMAKE_C_FLAGS of c-pal builds, so that rejects_with_warnings_as_errors.c fails also if CMAKE_C_FLAGS changes
    if(MSVC)
        list(APPEND definitions /WX)
    else()
        list(APPEND definitions -Werror)
    endif()

    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    set(CMAKE_TRY_COMPILE_CONFIGURATION ${CMAKE_BUILD_TYPE})

    foreach(source compiles rejects_with_errors rejects_with_warnings_as_errors)
        try_compile(UNIQUE_THANDLE_COMPILE_CHECK_${source} ${CMAKE_CURRENT_BINARY_DIR}/unique_thandle_compile_checks/${source}
            SOURCES ${checks_dir}/${source}.c
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${include_dirs}"
            COMPILE_DEFINITIONS ${definitions}
            OUTPUT_VARIABLE ${source}_output
        )
    endforeach()

    if(NOT UNIQUE_THANDLE_COMPILE_CHECK_compiles)
        message(FATAL_ERROR "UNIQUE_THANDLE compile checks: compiles.c, the documented use of UNIQUE_THANDLE, fails to compile:\n${compiles_output}")
    endif()

    unique_thandle_compile_checks_expect_cases(rejects_with_errors ${UNIQUE_THANDLE_COMPILE_CHECK_rejects_with_errors} "${rejects_with_errors_output}" "${error_cases}")
    unique_thandle_compile_checks_expect_cases(rejects_with_warnings_as_errors ${UNIQUE_THANDLE_COMPILE_CHECK_rejects_with_warnings_as_errors} "${rejects_with_warnings_as_errors_output}" "${warning_cases}")

    list(LENGTH error_cases error_case_count)
    list(LENGTH warning_cases warning_case_count)
    message(STATUS "UNIQUE_THANDLE compile checks: the documented use compiles, ${error_case_count} cases fail with errors and ${warning_case_count} cases fail with warnings as errors")
endfunction()
