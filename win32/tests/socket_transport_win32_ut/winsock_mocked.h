// Copyright (c) Microsoft. All rights reserved.

#ifndef WINSOCK_MOCKED_H
#define WINSOCK_MOCKED_H

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

#include "umock_c/umock_c_prod.h"

#define getaddrinfo mocked_getaddrinfo
#define connect mocked_connect
#define freeaddrinfo mocked_freeaddrinfo
#define socket mocked_socket
#define htons mocked_htons
#define bind mocked_bind
#define ioctlsocket mocked_ioctlsocket
#define listen mocked_listen
#define WSAGetLastError mocked_WSAGetLastError
#define select mocked_select
#define accept mocked_accept
#define closesocket mocked_closesocket
#define inet_ntop mocked_inet_ntop
#define WSARecv mocked_WSARecv
#define WSASend mocked_WSASend
#define __WSAFDIsSet mocked___WSAFDIsSet
#define ioctlsocket mocked_ioctlsocket

MOCKABLE_FUNCTION(, int, mocked_getaddrinfo, const char*, pNodeName, const char*, pServiceName, const ADDRINFOA*, pHints, PADDRINFOA*, ppResult);
MOCKABLE_FUNCTION(, int, mocked_connect, SOCKET, s, const struct sockaddr*, name, int, namelen);
MOCKABLE_FUNCTION(, void, mocked_freeaddrinfo, PADDRINFOA, pAddrInfo);
MOCKABLE_FUNCTION(, SOCKET, mocked_socket, int, af, int, type, int, protocol);
MOCKABLE_FUNCTION(, u_short, mocked_htons, unsigned short, hostshort);
MOCKABLE_FUNCTION(, int, mocked_bind, SOCKET, s, const struct sockaddr*, name, int, namelen);
MOCKABLE_FUNCTION(, int, mocked_ioctlsocket, SOCKET, s, long, cmd, u_long*, argp);
MOCKABLE_FUNCTION(, int, mocked_listen, SOCKET, s, int, backlog);
MOCKABLE_FUNCTION(, int, mocked_WSAGetLastError);
MOCKABLE_FUNCTION(, int, mocked_select, int, nfds, fd_set*, readfds, fd_set*, writefds, fd_set*, exceptfds, const struct timeval*, timeout);
MOCKABLE_FUNCTION(, SOCKET, mocked_accept, SOCKET, s, struct sockaddr*, addr, int*, addrlen);
MOCKABLE_FUNCTION(, int, mocked_closesocket, SOCKET, s);

MOCKABLE_FUNCTION(, PCSTR, mocked_inet_ntop, INT, Family, const VOID*, pAddr, PSTR, pStringBuf, size_t, StringBufSize);
MOCKABLE_FUNCTION(, int, mocked_WSARecv, SOCKET, s, LPWSABUF, lpBuffers, DWORD, dwBufferCount, LPDWORD, lpNumberOfBytesRecvd, LPDWORD, lpFlags, LPWSAOVERLAPPED, lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE, lpCompletionRoutine);
MOCKABLE_FUNCTION(, int, mocked_WSASend, SOCKET, s, LPWSABUF, lpBuffers, DWORD, dwBufferCount, LPDWORD, lpNumberOfBytesSent, DWORD, dwFlags, LPWSAOVERLAPPED, lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE, lpCompletionRoutine);
MOCKABLE_FUNCTION(, int, mocked___WSAFDIsSet, SOCKET, fd, fd_set*, p);
#endif // WINSOCK_MOCKED_H
