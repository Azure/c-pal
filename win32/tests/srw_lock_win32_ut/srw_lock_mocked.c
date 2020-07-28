// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#define InitializeSRWLock mocked_InitializeSRWLock
#define AcquireSRWLockExclusive mocked_AcquireSRWLockExclusive
#define TryAcquireSRWLockExclusive mocked_TryAcquireSRWLockExclusive
#define ReleaseSRWLockExclusive mocked_ReleaseSRWLockExclusive
#define AcquireSRWLockShared mocked_AcquireSRWLockShared
#define TryAcquireSRWLockShared mocked_TryAcquireSRWLockShared
#define ReleaseSRWLockShared mocked_ReleaseSRWLockShared

#ifdef __cplusplus
extern "C" {
#endif

void mocked_InitializeSRWLock(PSRWLOCK SRWLock);
void mocked_AcquireSRWLockExclusive(PSRWLOCK SRWLock);
BOOLEAN mocked_TryAcquireSRWLockExclusive(PSRWLOCK SRWLock);
void mocked_ReleaseSRWLockExclusive(PSRWLOCK SRWLock);
void mocked_AcquireSRWLockShared(PSRWLOCK SRWLock);
BOOLEAN mocked_TryAcquireSRWLockShared(PSRWLOCK SRWLock);
void mocked_ReleaseSRWLockShared(PSRWLOCK SRWLock);

#ifdef __cplusplus
}
#endif


#include "../../src/srw_lock_win32.c"

