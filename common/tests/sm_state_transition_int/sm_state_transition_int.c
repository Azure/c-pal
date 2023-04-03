// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_pal/timer.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"

#include "c_pal/interlocked.h"
#include "c_pal/sysinfo.h" // IWYU pragma: keep
#include "c_pal/sync.h" // IWYU pragma: keep
#include "c_pal/interlocked_hl.h"
#include "c_logging/xlogging.h"

#include "c_pal/sm.h"

#include "interface.h"
#include "ll.h"
#include "hl.h"
#include "hl_2.h"

#define XTEST_FUNCTION(x) void x(void)

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES);

#define N_MAX_THREADS 256

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

typedef struct TEST_INFO_CONTEXT_TAG
{
    EXECUTION_ENGINE_HANDLE execution_engine;
    THREADPOOL_HANDLE threadpool;
    TIMER_INSTANCE_HANDLE timer_handle;
    volatile_atomic int32_t open_complete_called;
} TEST_INFO_CONTEXT;


static ON_LL_OPEN_COMPLETE g_open_complete;
static void* g_open_complete_ctx;
static ON_LL_ERROR g_on_error;
static void* g_on_error_ctx;

static void wait_for_value(volatile_atomic int32_t* counter, int32_t target_value)
{
    int32_t value;
    while ((value = interlocked_add(counter, 0)) != target_value)
    {
        (void)wait_on_address(counter, value, UINT32_MAX);
    }
}

static void on_threadpool_open_complete(void* context, THREADPOOL_OPEN_RESULT open_result)
{
    volatile_atomic int32_t* threadpool_open = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_OK, open_result);

    (void)interlocked_increment(threadpool_open);
    wake_by_address_single(threadpool_open);
}

static void create_threadpool(TEST_INFO_CONTEXT* test_info)
{
    test_info->execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(test_info->execution_engine);
    test_info->threadpool = threadpool_create(test_info->execution_engine);
    ASSERT_IS_NOT_NULL(test_info->threadpool);

    volatile_atomic int32_t threadpool_open;
    (void)interlocked_exchange(&threadpool_open, 0);

    threadpool_open_async(test_info->threadpool, on_threadpool_open_complete, (void*)&threadpool_open);

    wait_for_value(&threadpool_open, 1);
}

static void threadtimer_complete(void* context)
{
    (void)context;
    g_open_complete(g_open_complete_ctx, true);
}

static void system_mock_api_open_async(void* api_ctx, ON_LL_OPEN_COMPLETE open_complete, void* open_complete_ctx, ON_LL_ERROR on_error, void* on_error_ctx)
{
    TEST_INFO_CONTEXT* test_info = (TEST_INFO_CONTEXT*)api_ctx;

    g_open_complete = open_complete;
    g_open_complete_ctx = open_complete_ctx;
    g_on_error = on_error;
    g_on_error_ctx = on_error_ctx;

    // Determine if the test is going to be a positive or negative test
    threadpool_timer_start(test_info->threadpool, 1000, 0, threadtimer_complete, test_info, &test_info->timer_handle);
}

static void system_mock_api_open_cancelled(void* api_ctx, ON_LL_OPEN_COMPLETE open_complete, void* open_complete_ctx, ON_LL_ERROR on_error, void* on_error_ctx)
{
    (void)open_complete;
    (void)open_complete_ctx;
    g_on_error = on_error;
    g_on_error_ctx = on_error_ctx;
    TEST_INFO_CONTEXT* test_info = (TEST_INFO_CONTEXT*)api_ctx;
    (void)interlocked_increment(&test_info->open_complete_called);
    wake_by_address_single(&test_info->open_complete_called);
}

static void system_mock_api_open_error(void* api_ctx, ON_LL_OPEN_COMPLETE open_complete, void* open_complete_ctx, ON_LL_ERROR on_error, void* on_error_ctx)
{
    (void)open_complete;
    (void)open_complete_ctx;
    (void)on_error;
    (void)on_error_ctx;
    TEST_INFO_CONTEXT* test_info = (TEST_INFO_CONTEXT*)api_ctx;
    (void)interlocked_increment(&test_info->open_complete_called);
    wake_by_address_single(&test_info->open_complete_called);
}

static void system_mock_api_close(void* api_ctx)
{
    TEST_INFO_CONTEXT* test_info = (TEST_INFO_CONTEXT*)api_ctx;
    if (test_info->timer_handle != NULL)
    {
        threadpool_timer_destroy(test_info->timer_handle);
    }
}

static void on_interface_open_complete(void* context, bool open_result)
{
    (void)open_result;
    TEST_INFO_CONTEXT* test_info = (TEST_INFO_CONTEXT*)context;

    (void)interlocked_increment(&test_info->open_complete_called);
    wake_by_address_single(&test_info->open_complete_called);
}

static void on_interface_error(void* context)
{
    (void)context;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_initialize)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(sm_state_transition_happy_path)
{
    TEST_INFO_CONTEXT test_info = { 0 };
    (void)interlocked_exchange(&test_info.open_complete_called, 0);
    create_threadpool(&test_info);

    // Get the HL information
    const INTERFACE_DESCRIPTION* hl_interface = hl_get_interface_description();

    LL_IO_CONFIG ll_config;
    HL_IO_CONFIG hl_config;

    ll_config.sys_api_open_async = system_mock_api_open_async;
    ll_config.sys_api_open_async_ctx = &test_info;
    ll_config.sys_api_close = system_mock_api_close;
    ll_config.sys_api_close_ctx = &test_info;

    hl_config.underlying_interface = ll_get_interface_description();
    hl_config.config_param = &ll_config;

    CONCRETE_INTERFACE_HANDLE hl_handle = hl_interface->create(&hl_config);
    ASSERT_IS_NOT_NULL(hl_handle);

    ASSERT_ARE_EQUAL(int, 0, hl_interface->open_async(hl_handle, on_interface_open_complete, &test_info, on_interface_error, &test_info));

    wait_for_value(&test_info.open_complete_called, 1);

    hl_interface->close(hl_handle);

    hl_interface->destroy(hl_handle);

    threadpool_close(test_info.threadpool);
    threadpool_destroy(test_info.threadpool);
    execution_engine_dec_ref(test_info.execution_engine);
}

TEST_FUNCTION(sm_state_open_cancelled)
{
    TEST_INFO_CONTEXT test_info = { 0 };
    (void)interlocked_exchange(&test_info.open_complete_called, 0);
    create_threadpool(&test_info);

    // Get the HL information
    const INTERFACE_DESCRIPTION* hl_interface = hl_get_interface_description();

    LL_IO_CONFIG ll_config;
    HL_IO_CONFIG hl_config;

    ll_config.sys_api_open_async = system_mock_api_open_cancelled;
    ll_config.sys_api_open_async_ctx = &test_info;
    ll_config.sys_api_close = system_mock_api_close;
    ll_config.sys_api_close_ctx = &test_info;

    hl_config.underlying_interface = ll_get_interface_description();
    hl_config.config_param = &ll_config;

    CONCRETE_INTERFACE_HANDLE hl_handle = hl_interface->create(&hl_config);
    ASSERT_IS_NOT_NULL(hl_handle);

    ASSERT_ARE_EQUAL(int, 0, hl_interface->open_async(hl_handle, on_interface_open_complete, &test_info, on_interface_error, &test_info));

    wait_for_value(&test_info.open_complete_called, 1);

    hl_interface->close(hl_handle);

    hl_interface->destroy(hl_handle);

    threadpool_close(test_info.threadpool);
    threadpool_destroy(test_info.threadpool);
    execution_engine_dec_ref(test_info.execution_engine);
}

TEST_FUNCTION(sm_state_open_ll_error)
{
    TEST_INFO_CONTEXT test_info = { 0 };
    (void)interlocked_exchange(&test_info.open_complete_called, 0);
    create_threadpool(&test_info);

    // Get the HL information
    const INTERFACE_DESCRIPTION* hl_interface = hl_get_interface_description();

    LL_IO_CONFIG ll_config;
    HL_IO_CONFIG hl_config;

    ll_config.sys_api_open_async = system_mock_api_open_cancelled;
    ll_config.sys_api_open_async_ctx = &test_info;
    ll_config.sys_api_close = system_mock_api_close;
    ll_config.sys_api_close_ctx = &test_info;

    hl_config.underlying_interface = ll_get_interface_description();
    hl_config.config_param = &ll_config;

    CONCRETE_INTERFACE_HANDLE hl_handle = hl_interface->create(&hl_config);
    ASSERT_IS_NOT_NULL(hl_handle);

    ASSERT_ARE_EQUAL(int, 0, hl_interface->open_async(hl_handle, on_interface_open_complete, &test_info, on_interface_error, &test_info));

    wait_for_value(&test_info.open_complete_called, 1);

    g_on_error(g_on_error_ctx);

    hl_interface->close(hl_handle);

    hl_interface->destroy(hl_handle);

    threadpool_close(test_info.threadpool);
    threadpool_destroy(test_info.threadpool);
    execution_engine_dec_ref(test_info.execution_engine);
}

TEST_FUNCTION(sm_state_open_two_ll_components)
{
    TEST_INFO_CONTEXT test_info = { 0 };
    (void)interlocked_exchange(&test_info.open_complete_called, 0);
    create_threadpool(&test_info);

    // Get the HL information
    const INTERFACE_DESCRIPTION* hl_interface = hl_get_interface_description();

    LL_IO_CONFIG ll_config;
    HL_IO_CONFIG hl_config;

    ll_config.sys_api_open_async = system_mock_api_open_cancelled;
    ll_config.sys_api_open_async_ctx = &test_info;
    ll_config.sys_api_close = system_mock_api_close;
    ll_config.sys_api_close_ctx = &test_info;

    hl_config.underlying_interface = ll_get_interface_description();
    hl_config.config_param = &ll_config;

    CONCRETE_INTERFACE_HANDLE hl_handle = hl_interface->create(&hl_config);
    ASSERT_IS_NOT_NULL(hl_handle);

    ASSERT_ARE_EQUAL(int, 0, hl_interface->open_async(hl_handle, on_interface_open_complete, &test_info, on_interface_error, &test_info));

    wait_for_value(&test_info.open_complete_called, 1);

    hl_interface->close(hl_handle);

    hl_interface->destroy(hl_handle);

    threadpool_close(test_info.threadpool);
    threadpool_destroy(test_info.threadpool);
    execution_engine_dec_ref(test_info.execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
