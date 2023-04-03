
// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>

#include "testrunnerswitcher.h"

#include "c_logging/xlogging.h"

#include "c_pal/sm.h"

#include "interface.h"
#include "ll.h"

typedef struct LL_IO_TAG
{
    SM_HANDLE sm;

    SYSTEM_MOCK_API_OPEN_ASYNC sys_api_open_async;
    void* sys_api_open_async_ctx;
    SYSTEM_MOCK_API_CLOSE sys_api_close;
    void* sys_api_close_ctx;


    ON_INTERFACE_OPEN_COMPLETE on_open_complete;
    void* on_open_complete_ctx;
    ON_INTERFACE_ERROR on_error;
    void* on_error_ctx;
} LL_IO;

static void on_sm_closing_complete(void* context)
{
    (void)context;
}

static void on_sm_closing_while_opening(void* context)
{
    (void)context;
    LogInfo("LL sm_close called while in the OPENING state");

    // Should I call open complete with failure?
    LL_IO* ll_io = (LL_IO*)context;
    ll_io->on_open_complete(ll_io->on_open_complete_ctx, false);
}

static void on_ll_open_complete(void* context, bool open_result)
{
    LL_IO* ll_io = (LL_IO*)context;

    ll_io->on_open_complete(ll_io->on_open_complete_ctx, open_result);

    sm_open_end(ll_io->sm, open_result);
}

static void on_ll_error(void* context)
{
    LL_IO* ll_io = (LL_IO*)context;

    sm_fault(ll_io->sm);

    ll_io->on_error(ll_io->on_error_ctx);
}

static CONCRETE_INTERFACE_HANDLE ll_create(const void* interface_parameters)
{
    LL_IO* result = malloc(sizeof(LL_IO));
    ASSERT_IS_NOT_NULL(result);

    result->sm = sm_create("LL_Layer");
    ASSERT_IS_NOT_NULL(result->sm);

    LL_IO_CONFIG* ll_io = (LL_IO_CONFIG*)interface_parameters;
    result->sys_api_open_async = ll_io->sys_api_open_async;
    result->sys_api_open_async_ctx = ll_io->sys_api_open_async_ctx;
    result->sys_api_close = ll_io->sys_api_close;
    result->sys_api_close_ctx = ll_io->sys_api_close_ctx;

    return result;
}

static void ll_destroy(CONCRETE_INTERFACE_HANDLE handle)
{
    LL_IO* ll_io = (LL_IO*)handle;
    sm_destroy(ll_io->sm);
    free(ll_io);
}

static int ll_open_async(CONCRETE_INTERFACE_HANDLE handle, ON_INTERFACE_OPEN_COMPLETE on_open_complete, void* on_open_complete_ctx, ON_INTERFACE_ERROR on_error, void* on_error_ctx)
{
    int result;

    LL_IO* ll_io = (LL_IO*)handle;

    SM_RESULT sm_result = sm_open_begin(ll_io->sm);
    if (sm_result == SM_EXEC_REFUSED)
    {
        result = MU_FAILURE;
    }
    else
    {
        ll_io->on_open_complete = on_open_complete;
        ll_io->on_open_complete_ctx = on_open_complete_ctx;
        ll_io->on_error = on_error;
        ll_io->on_error_ctx = on_error_ctx;

        ll_io->sys_api_open_async(ll_io->sys_api_open_async_ctx, on_ll_open_complete, ll_io, on_ll_error, ll_io);
        result = 0;
    }
    return result;
}

static void ll_close(CONCRETE_INTERFACE_HANDLE handle)
{
    LL_IO* ll_io = (LL_IO*)handle;
    SM_RESULT sm_result = sm_close_begin_with_cb(ll_io->sm, on_sm_closing_complete, ll_io, on_sm_closing_while_opening, ll_io);
    if (sm_result == SM_EXEC_REFUSED)
    {
        LogWarning("ll_close sm result was %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
    }
    else
    {
        // Close the rest of the items
        ll_io->sys_api_close(ll_io->sys_api_close_ctx);
    }
}

const INTERFACE_DESCRIPTION ll_interface_desc =
{
    ll_create,
    ll_destroy,
    ll_open_async,
    ll_close
};

const INTERFACE_DESCRIPTION* ll_get_interface_description(void)
{
    return &ll_interface_desc;
}
