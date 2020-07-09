#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/gballoc.h
    ../common/inc/constbuffer.h
    PARENT_SCOPE
)

set(pal_common_c_files
    ../common/src/gballoc.c
    ../common/src/constbuffer.c
    PARENT_SCOPE
)

FILE(GLOB pal_common_md_files_internal "devdoc/*.md")
set(pal_common_md_files ${pal_common_md_files_internal} PARENT_SCOPE)
