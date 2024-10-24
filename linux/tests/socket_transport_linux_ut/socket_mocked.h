// Copyright (c) Microsoft. All rights reserved.

#ifndef SOCKET_MOCKED_H
#define SOCKET_MOCKED_H

#include <stddef.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>

#include "umock_c/umock_c_prod.h"

#include "c_pal/socket_handle.h"

struct addrinfo;

#define inet_ntop mocked_inet_ntop
#define getaddrinfo mocked_getaddrinfo
#define freeaddrinfo mocked_freeaddrinfo
#define connect mocked_connect
#define socket mocked_socket
#define htons mocked_htons
#define bind mocked_bind
#define ioctlsocket mocked_ioctlsocket
#define setsockopt mock_setsockopt
#define listen mocked_listen
#define send mocked_send
#define recv mocked_recv
#define accept mocked_accept
#define close mocked_close
#define shutdown mocked_shutdown
#define gethostname mocked_gethostname
#define getifaddrs mocked_getifaddrs
#define freeifaddrs mocked_freeifaddrs
#define getnameinfo mocked_getnameinfo
#define fcntl mocked_fcntl

MOCKABLE_FUNCTION(, const char*, mocked_inet_ntop, int, af, const void*, cp, char*, buf, socklen_t, len);
MOCKABLE_FUNCTION(, int, mocked_getaddrinfo, const char*, pNodeName, const char*, pServiceName, const struct addrinfo*, pHints, struct addrinfo**, ppResult);
MOCKABLE_FUNCTION(, void, mocked_freeaddrinfo, struct addrinfo*, pAddrInfo);
MOCKABLE_FUNCTION(, int, mocked_connect, SOCKET_HANDLE, s, const struct sockaddr*, name, int, namelen);
MOCKABLE_FUNCTION(, SOCKET_HANDLE, mocked_socket, int, af, int, type, int, protocol);
MOCKABLE_FUNCTION(, u_short, mocked_htons, unsigned short, hostshort);
MOCKABLE_FUNCTION(, int, mocked_bind, SOCKET_HANDLE, s, const struct sockaddr*, name, int, namelen);
MOCKABLE_FUNCTION(, int, mocked_ioctlsocket, int, s, long, cmd, u_long*, argp);
MOCKABLE_FUNCTION(, int, mock_setsockopt, int, fd, int, __level, int, __optname, const void*, __optval, socklen_t, __optlen);
MOCKABLE_FUNCTION(, int, mocked_listen, SOCKET_HANDLE, s, int, backlog);
MOCKABLE_FUNCTION(, ssize_t, mocked_send, SOCKET_HANDLE, sockfd, const void*, buf, size_t, len, int, flags);
MOCKABLE_FUNCTION(, ssize_t, mocked_recv, SOCKET_HANDLE, sockfd, void*, buf, size_t, len, int, flags);
MOCKABLE_FUNCTION(, SOCKET_HANDLE, mocked_accept, SOCKET_HANDLE, s, struct sockaddr*, addr, socklen_t*, addrlen);
MOCKABLE_FUNCTION(, int, mocked_close, SOCKET_HANDLE, s);
MOCKABLE_FUNCTION(, int, mocked_shutdown, SOCKET_HANDLE, __fd, int, __how);
MOCKABLE_FUNCTION(, int, mocked_gethostname, char*, name, size_t, namelen);
MOCKABLE_FUNCTION(, int, mocked_getifaddrs, struct ifaddrs**, ifap);
MOCKABLE_FUNCTION(, void, mocked_freeifaddrs, struct ifaddrs*, ifap);
MOCKABLE_FUNCTION(, int, mocked_getnameinfo, const struct sockaddr*, addr, socklen_t, addrlen, char*, host, socklen_t, hostlen, char*, serv, socklen_t, servlen, int, flags);
extern int mocked_fcntl(int __fd, int __cmd, ...);

#endif // SOCKET_MOCKED_H
