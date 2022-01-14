// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_UUID_H
#define REAL_UUID_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_UUID_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        uuid_produce                                    \
    )

#ifdef __cplusplus
extern "C" {
#endif

    int real_uuid_produce(UUID_T destination);

#ifdef WIN32 /*some functions only exists in Windows realm*/
    int real_uuid_from_GUID(UUID_T destination, const GUID* source);
    int real_GUID_from_uuid(GUID* destination, const UUID_T source);
#endif

#ifdef __cplusplus
}
#endif

#endif //REAL_UUID_H
