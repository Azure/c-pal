// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_pal/socket_handle.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

typedef struct NET_COMM_INSTANCE_TAG* NET_COMM_INSTANCE_HANDLE;
typedef void* COMM_HANDLE;

#define IO_SEND_RESULT_VALUES \
    IO_SEND_OK, \
    IO_SEND_ERROR,  \
    IO_SEND_CANCELLED
MU_DEFINE_ENUM_WITHOUT_INVALID(IO_SEND_RESULT, IO_SEND_RESULT_VALUES)

#define IO_OPEN_RESULT_VALUES \
    IO_OPEN_OK, \
    IO_OPEN_ERROR, \
    IO_OPEN_CANCELLED
MU_DEFINE_ENUM_WITHOUT_INVALID(IO_OPEN_RESULT, IO_OPEN_RESULT_VALUES)

#define IO_ERROR_RESULT_VALUES \
    IO_ERROR_OK, \
    IO_ERROR_GENERAL, \
    IO_ERROR_MEMORY, \
    IO_ERROR_ENDPOINT_DISCONN
MU_DEFINE_ENUM_WITHOUT_INVALID(IO_ERROR_RESULT, IO_ERROR_RESULT_VALUES)

#define NET_ADDRESS_TYPE_VALUES \
    ADDRESS_TYPE_IP, \
    ADDRESS_TYPE_UDP
MU_DEFINE_ENUM_WITHOUT_INVALID(NET_ADDRESS_TYPE, NET_ADDRESS_TYPE_VALUES)

typedef void(*ON_BYTES_RECEIVED)(void* context, const unsigned char* buffer, size_t size);
typedef void(*ON_SEND_COMPLETE)(void* context, IO_SEND_RESULT send_result);
typedef void(*ON_IO_OPEN_COMPLETE)(void* context, IO_OPEN_RESULT open_result);
typedef void(*ON_IO_CLOSE_COMPLETE)(void* context);
typedef void(*ON_IO_ERROR)(void* context, IO_ERROR_RESULT error_result);
typedef void(*ON_INCOMING_CONNECT)(void* context, const void* config);

typedef struct COMM_CALLBACK_INFO_TAG
{
    ON_BYTES_RECEIVED on_bytes_received;
    void* on_bytes_received_ctx;
    ON_IO_CLOSE_COMPLETE on_io_close_complete;
    void* on_close_ctx;
    ON_IO_ERROR on_io_error;
    void* on_io_error_ctx;
} COMM_CALLBACK_INFO;

typedef COMM_HANDLE(*COMM_CREATE)(const void* io_create_parameters, const COMM_CALLBACK_INFO* client_cb);
typedef void(*COMM_DESTROY)(COMM_HANDLE impl_handle);
typedef int(*COMM_OPEN)(COMM_HANDLE impl_handle, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_ctx);
typedef int(*COMM_CLOSE)(COMM_HANDLE impl_handle);
typedef int(*COMM_SEND)(COMM_HANDLE impl_handle, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
typedef int(*COMM_SEND_NOTIFY)(COMM_HANDLE impl_handle, const void* buffer, uint32_t size, uint32_t flags, void* overlapped_info);
typedef int(*COMM_RECV)(COMM_HANDLE impl_handle);
typedef int(*COMM_RECV_NOTIFY)(COMM_HANDLE impl_handle, void* buffer, uint32_t size, uint32_t* flags, void* overlapped_info);
typedef SOCKET_HANDLE(*COMM_UNDERLYING_HANDLE)(COMM_HANDLE impl_handle);

typedef int(*COMM_LISTEN)(COMM_HANDLE impl_handle);
typedef int(*COMM_ACCEPT_CONN)(COMM_HANDLE impl_handle);
typedef COMM_HANDLE(*COMM_ACCEPT_NOTIFY)(COMM_HANDLE impl_handle, const COMM_CALLBACK_INFO* client_cb);

typedef struct COMM_INTERFACE_DESCRIPTION_TAG
{
    COMM_CREATE interface_impl_create;
    COMM_DESTROY interface_impl_destroy;
    COMM_OPEN interface_impl_open;
    COMM_CLOSE interface_impl_close;
    COMM_SEND interface_impl_send;
    COMM_SEND_NOTIFY interface_impl_send_notify;
    COMM_RECV interface_impl_recv;
    COMM_RECV_NOTIFY interface_impl_recv_notify;
    COMM_UNDERLYING_HANDLE interface_impl_underlying;
    COMM_LISTEN interface_impl_listen;
    COMM_ACCEPT_CONN interface_impl_accept_conn;
    COMM_ACCEPT_NOTIFY interface_impl_accept_notify;
} COMM_INTERFACE_DESCRIPTION;

MOCKABLE_FUNCTION(, NET_COMM_INSTANCE_HANDLE, net_comm_create, const COMM_INTERFACE_DESCRIPTION*, comm_description, const void*, parameters, const COMM_CALLBACK_INFO*, client_cb);
MOCKABLE_FUNCTION(, void, net_comm_destroy, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_open, NET_COMM_INSTANCE_HANDLE, handle, ON_IO_OPEN_COMPLETE, on_io_open_complete, void*, user_ctx);
MOCKABLE_FUNCTION(, int, net_comm_close, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_send, NET_COMM_INSTANCE_HANDLE, handle, const void*, buffer, size_t, size, ON_SEND_COMPLETE, on_send_complete, void*, user_ctx);
MOCKABLE_FUNCTION(, int, net_comm_send_notify, NET_COMM_INSTANCE_HANDLE, handle, const void*, buffer, uint32_t, size, uint32_t, flags, void*, overlapped_info);
MOCKABLE_FUNCTION(, int, net_comm_recv, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_recv_notify, NET_COMM_INSTANCE_HANDLE, handle, void*, buffer, uint32_t, size, uint32_t*, flags, void*, overlapped_info);
MOCKABLE_FUNCTION(, SOCKET_HANDLE, net_comm_underlying_handle, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_listen, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, net_comm_accept_conn, NET_COMM_INSTANCE_HANDLE, handle);
MOCKABLE_FUNCTION(, NET_COMM_INSTANCE_HANDLE, net_comm_accept_notify, NET_COMM_INSTANCE_HANDLE, handle, const COMM_CALLBACK_INFO*, client_cb);


#ifdef __cplusplus
}
#endif

