#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(c_pal_valgrind_suppressions_parameters
    "--suppressions=${CMAKE_CURRENT_LIST_DIR}/c-pal_suppressions.sup"
)

# helgrind versions that do not know the Dubious kind (e.g. 3.18) refuse to start with it in a suppressions file
if(UNIX AND run_helgrind)
    find_program(VALGRIND_FOUND NAMES valgrind)
    if(${VALGRIND_FOUND} STREQUAL VALGRIND_FOUND-NOTFOUND)
        # no valgrind, no helgrind tests
    else()
        execute_process(
            COMMAND valgrind --tool=helgrind "--suppressions=${CMAKE_CURRENT_LIST_DIR}/c-pal_helgrind_dubious_suppressions.sup" true
            RESULT_VARIABLE helgrind_dubious_suppressions_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT ${helgrind_dubious_suppressions_result} EQUAL 0)
            message(STATUS "helgrind does not support Helgrind:Dubious suppressions, not using c-pal_helgrind_dubious_suppressions.sup")
        else()
            list(APPEND c_pal_valgrind_suppressions_parameters "--suppressions=${CMAKE_CURRENT_LIST_DIR}/c-pal_helgrind_dubious_suppressions.sup")
        endif()
    endif()
else()
    # helgrind tests are not run
endif()

set(VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER
    ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER}
    ${c_pal_valgrind_suppressions_parameters} PARENT_SCOPE
)
