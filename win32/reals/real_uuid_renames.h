// Copyright (c) Microsoft. All rights reserved.

#define uuid_produce real_uuid_produce

#ifdef WIN32 /*some functions only exists in Windows realm*/
#define uuid_from_GUID real_uuid_from_GUID
#endif

#define UUID_T_FROM_STRING_RESULT real_UUID_T_FROM_STRING_RESULT
