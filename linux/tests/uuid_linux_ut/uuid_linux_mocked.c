// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <uuid/uuid.h>

#define uuid_generate mocked_uuid_generate

void mocked_uuid_generate(uuid_t out);

#include "../../src/uuid_linux.c"
