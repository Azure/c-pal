// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SOCKET_HANDLE_H
#define SOCKET_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* this type should abstract a socket for different platforms. I.e. for Windows it should simply wrap SOCKET */
#ifdef WIN32
    typedef void* SOCKET_HANDLE;
    #define PRI_SOCKET "p"
    #define SOCKET_ASSERT_TYPE void_ptr
#else
    typedef int SOCKET_HANDLE;
    #define INVALID_SOCKET  -1
    #define PRI_SOCKET "d"
    #define SOCKET_ASSERT_TYPE int
#endif

#ifdef __cplusplus
}
#endif

#endif // SOCKET_HANDLE_H
