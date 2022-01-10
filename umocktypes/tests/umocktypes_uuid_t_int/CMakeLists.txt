#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName umocktypes_uuid_t_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    module.c
)
set(${theseTestsName}_h_files
    module.h
    submodule.h
)

build_test_artifacts(${theseTestsName} "tests/c_pal/umocktypes" ADDITIONAL_LIBS c_pal_umocktypes c_pal)
