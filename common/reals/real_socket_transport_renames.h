// Copyright (c) Microsoft. All rights reserved.

#define socket_transport_create_client             real_socket_transport_create_client
#define socket_transport_create_server             real_socket_transport_create_server
#define socket_transport_create_from_socket        real_socket_transport_create_from_socket
#define socket_transport_destroy                   real_socket_transport_destroy
#define socket_transport_connect                   real_socket_transport_connect
#define socket_transport_listen                    real_socket_transport_listen
#define socket_transport_disconnect                real_socket_transport_disconnect
#define socket_transport_accept                    real_socket_transport_accept
#define socket_transport_send                      real_socket_transport_send
#define socket_transport_receive                   real_socket_transport_receive
#define socket_transport_get_underlying_socket     real_socket_transport_get_underlying_socket
#define socket_transport_is_valid_socket           real_socket_transport_is_valid_socket
#define socket_transport_get_local_address         real_socket_transport_get_local_address

#define SOCKET_SEND_RESULT                         real_SOCKET_SEND_RESULT
#define SOCKET_RECEIVE_RESULT                      real_SOCKET_RECEIVE_RESULT
#define SOCKET_ACCEPT_RESULT                       real_SOCKET_ACCEPT_RESULT
#define SOCKET_TYPE                                real_SOCKET_TYPE
#define SOCKET_IO_TYPE                             real_SOCKET_IO_TYPE
#define ADDRESS_TYPE                               real_ADDRESS_TYPE
