
// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>

#include "testrunnerswitcher.h"

#include "c_logging/xlogging.h"

#include "c_pal/sm.h"

#include "interface.h"
#include "hl.h"

typedef struct HL_IO_TAG
{
    SM_HANDLE sm;
    const INTERFACE_DESCRIPTION* interface_desc;

    CONCRETE_INTERFACE_HANDLE underlying_io;
    ON_INTERFACE_OPEN_COMPLETE on_open_complete;
    void* on_open_complete_ctx;
    ON_INTERFACE_ERROR on_error;
    void* on_error_ctx;
} HL_IO;

static void on_sm_closing_complete(void* context)
{
    (void)context;
}

static void on_sm_closing_while_opening(void* context)
{
    HL_IO* hl_io = (HL_IO*)context;
    LogInfo("HL sm_close called while in the OPENING state closing the underlying Io");

    hl_io->interface_desc->close(hl_io->underlying_io);
}

static void on_open_complete_cb(void* context, bool open_success)
{
    HL_IO* hl_io = (HL_IO*)context;

    LogInfo("HL on_open_complete_cb has been called with %" PRI_BOOL " calling open_complete", MU_BOOL_VALUE(open_success));
    hl_io->on_open_complete(hl_io->on_open_complete_ctx, open_success);
    sm_open_end(hl_io->sm, open_success);
}

static void on_error_cb(void* context)
{
    HL_IO* hl_io = (HL_IO*)context;
    sm_fault(hl_io->sm);

    hl_io->on_error(hl_io->on_error_ctx);
}

static CONCRETE_INTERFACE_HANDLE hl_create(const void* interface_parameters)
{
    HL_IO* result = malloc(sizeof(HL_IO));
    ASSERT_IS_NOT_NULL(result);

    HL_IO_CONFIG* hl_config = (HL_IO_CONFIG*)interface_parameters;
    result->interface_desc = hl_config->underlying_interface;

    result->sm = sm_create("HL_Layer");
    ASSERT_IS_NOT_NULL(result->sm);

    // Create the underlying IO
    result->underlying_io = result->interface_desc->create(hl_config->config_param);
    ASSERT_IS_NOT_NULL(result->underlying_io);

    return result;
}

static void hl_destroy(CONCRETE_INTERFACE_HANDLE handle)
{
    HL_IO* hl_io = (HL_IO*)handle;

    hl_io->interface_desc->destroy(hl_io->underlying_io);

    sm_destroy(hl_io->sm);
    free(hl_io);
}

static int hl_open_async(CONCRETE_INTERFACE_HANDLE handle, ON_INTERFACE_OPEN_COMPLETE on_open_complete, void* on_open_complete_ctx, ON_INTERFACE_ERROR on_error, void* on_error_ctx)
{
    int result;

    HL_IO* hl_io = (HL_IO*)handle;

    hl_io->on_open_complete = on_open_complete;
    hl_io->on_open_complete_ctx = on_open_complete_ctx;
    hl_io->on_error = on_error;
    hl_io->on_error_ctx = on_error_ctx;

    SM_RESULT sm_result = sm_open_begin(hl_io->sm);
    if (sm_result == SM_EXEC_REFUSED)
    {
        result = MU_FAILURE;
    }
    else
    {
        ASSERT_ARE_EQUAL(int, 0, hl_io->interface_desc->open_async(hl_io->underlying_io, on_open_complete_cb, hl_io, on_error_cb, hl_io));
        result = 0;
    }

    return result;
}

static void hl_close(CONCRETE_INTERFACE_HANDLE handle)
{
    HL_IO* hl_io = (HL_IO*)handle;

    SM_RESULT sm_result = sm_close_begin_with_cb(hl_io->sm, on_sm_closing_complete, hl_io, on_sm_closing_while_opening, hl_io);
    if (sm_result == SM_EXEC_REFUSED)
    {
        LogWarning("hl_close sm result was %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
    }
    else
    {
        hl_io->interface_desc->close(hl_io->underlying_io);
    }
}

const INTERFACE_DESCRIPTION hl_interface_desc =
{
    hl_create,
    hl_destroy,
    hl_open_async,
    hl_close
};

const INTERFACE_DESCRIPTION* hl_get_interface_description(void)
{
    return &hl_interface_desc;
}
