// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <uuid.h>

#define uuid_create mocked_uuid_create
#define uuid_make mocked_uuid_make
#define uuid_export mocked_uuid_export
#define uuid_destroy mocked_uuid_destroy

uuid_rc_t mocked_uuid_create(uuid_t** out);
uuid_rc_t mocked_uuid_make(uuid_t* out, unsigned int mode);
uuid_rc_t mocked_uuid_export(const uuid_t *uuid, uuid_fmt_t fmt, void *data_ptr, size_t *data_len);
uuid_rc_t mocked_uuid_destroy(uuid_t* out);

#include "../../src/uuid_linux.c"
