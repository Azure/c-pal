#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName mocked_new_delete_ut)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

build_test_artifacts(${theseTestsName} "tests/c_pal/test_helpers" ADDITIONAL_LIBS c_pal_mocked_new_delete c_pal_reals
    MOCK_PRECOMPILE_HEADERS
        "c_pal/srw_lock.h"
        "c_pal_test_helpers/mocked_new_delete.h"
    NO_MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
)
