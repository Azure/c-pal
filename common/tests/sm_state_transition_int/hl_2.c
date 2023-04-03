
// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>

#include "testrunnerswitcher.h"

#include "c_logging/xlogging.h"

#include "c_pal/sm.h"

#include "interface.h"
#include "hl_2.h"

typedef struct HL_IO_TAG
{
    SM_HANDLE sm;
    const INTERFACE_DESCRIPTION* interface_desc;
    const INTERFACE_DESCRIPTION* interface_desc_2;

    CONCRETE_INTERFACE_HANDLE underlying_io;
    ON_INTERFACE_OPEN_COMPLETE on_open_complete;
    void* on_open_complete_ctx;
    ON_INTERFACE_ERROR on_error;
    void* on_error_ctx;

    CONCRETE_INTERFACE_HANDLE underlying_io_2;
    ON_INTERFACE_OPEN_COMPLETE on_open_complete_2;
    void* on_open_complete_ctx_2;
    ON_INTERFACE_ERROR on_error_2;
    void* on_error_ctx_2;

} HL_IO;

static void on_sm_closing_complete(void* context)
{
    (void)context;
}

static void on_sm_closing_while_opening(void* context)
{
    HL_IO* hl_2_io = (HL_IO*)context;
    LogInfo("HL sm_close called while in the OPENING state closing the underlying Io");

    hl_2_io->interface_desc->close(hl_2_io->underlying_io);
}

static void on_open_complete_cb(void* context, bool open_success)
{
    HL_IO* hl_2_io = (HL_IO*)context;

    LogInfo("HL on_open_complete_cb has been called with %" PRI_BOOL " calling open_complete", MU_BOOL_VALUE(open_success));
    hl_2_io->on_open_complete(hl_2_io->on_open_complete_ctx, open_success);
    sm_open_end(hl_2_io->sm, open_success);
}

static void on_error_cb(void* context)
{
    HL_IO* hl_2_io = (HL_IO*)context;
    sm_fault(hl_2_io->sm);

    hl_2_io->on_error(hl_2_io->on_error_ctx);
}

static CONCRETE_INTERFACE_HANDLE hl_2_create(const void* interface_parameters)
{
    HL_IO* result = malloc(sizeof(HL_IO));
    ASSERT_IS_NOT_NULL(result);

    HL_IO_CONFIG* hl_2_config = (HL_IO_CONFIG*)interface_parameters;
    result->interface_desc = hl_2_config->underlying_interface;
    result->interface_desc_2 = hl_2_config->underlying_interface;

    result->sm = sm_create("HL_2_Layer");
    ASSERT_IS_NOT_NULL(result->sm);

    // Create the underlying IO
    result->underlying_io = result->interface_desc->create(hl_2_config->config_param);
    ASSERT_IS_NOT_NULL(result->underlying_io);

    // Create the 2nd interface
    result->underlying_io_2 = result->interface_desc_2->create(hl_2_config->config_param);
    ASSERT_IS_NOT_NULL(result->underlying_io_2);

    return result;
}

static void hl_2_destroy(CONCRETE_INTERFACE_HANDLE handle)
{
    HL_IO* hl_2_io = (HL_IO*)handle;

    hl_2_io->interface_desc->destroy(hl_2_io->underlying_io);

    sm_destroy(hl_2_io->sm);
    free(hl_2_io);
}

static int hl_2_open_async(CONCRETE_INTERFACE_HANDLE handle, ON_INTERFACE_OPEN_COMPLETE on_open_complete, void* on_open_complete_ctx, ON_INTERFACE_ERROR on_error, void* on_error_ctx)
{
    int result;

    HL_IO* hl_2_io = (HL_IO*)handle;

    hl_2_io->on_open_complete = on_open_complete;
    hl_2_io->on_open_complete_ctx = on_open_complete_ctx;
    hl_2_io->on_error = on_error;
    hl_2_io->on_error_ctx = on_error_ctx;

    SM_RESULT sm_result = sm_open_begin(hl_2_io->sm);
    if (sm_result == SM_EXEC_REFUSED)
    {
        result = MU_FAILURE;
    }
    else
    {
        ASSERT_ARE_EQUAL(int, 0, hl_2_io->interface_desc->open_async(hl_2_io->underlying_io, on_open_complete_cb, hl_2_io, on_error_cb, hl_2_io));
        result = 0;
    }

    return result;
}

static void hl_2_close(CONCRETE_INTERFACE_HANDLE handle)
{
    HL_IO* hl_2_io = (HL_IO*)handle;

    SM_RESULT sm_result = sm_close_begin_with_cb(hl_2_io->sm, on_sm_closing_complete, hl_2_io, on_sm_closing_while_opening, hl_2_io);
    if (sm_result == SM_EXEC_REFUSED)
    {
        LogWarning("hl_2_close sm result was %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
    }
    else
    {
        hl_2_io->interface_desc->close(hl_2_io->underlying_io);
    }
}

const INTERFACE_DESCRIPTION hl_2_interface_desc =
{
    hl_2_create,
    hl_2_destroy,
    hl_2_open_async,
    hl_2_close
};

const INTERFACE_DESCRIPTION* hl_2_get_interface_description(void)
{
    return &hl_2_interface_desc;
}
