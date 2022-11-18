#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER
    ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER}
    --suppressions=${CMAKE_CURRENT_LIST_DIR}/c-pal_suppressions.sup
)