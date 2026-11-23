# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName mocked_new_delete_ut)

set(${theseTestsName}_test_files)
set(${theseTestsName}_c_files)
set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

build_test_artifacts(${theseTestsName} "tests/c_pal"
    ADDITIONAL_LIBS c_pal_mocked_new_delete c_pal_reals
)
