// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SOCKET_HANDLE_H
#define SOCKET_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* this type should abstract a socket for different platforms. I.e. for Windows it should simply wrap SOCKET */
typedef void* SOCKET_HANDLE;

#ifdef __cplusplus
}
#endif

#endif // SOCKET_HANDLE_H
