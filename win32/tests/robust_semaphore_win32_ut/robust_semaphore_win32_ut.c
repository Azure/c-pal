// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "robust_semaphore_win32_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(WINAPI, BOOL, mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA, LPCSTR, StringSecurityDescriptor, DWORD, StringSDRevision, PSECURITY_DESCRIPTOR*, SecurityDescriptor, PULONG, SecurityDescriptorSize);
MOCKABLE_FUNCTION(WINAPI, HLOCAL, mocked_LocalFree, HLOCAL, hMem);
MOCKABLE_FUNCTION(WINAPI, HANDLE, mocked_CreateMutexExA, LPSECURITY_ATTRIBUTES, lpMutexAttributes, LPCSTR, lpName, DWORD, dwFlags, DWORD, dwDesiredAccess);
MOCKABLE_FUNCTION(WINAPI, HANDLE, mocked_CreateEventA, LPSECURITY_ATTRIBUTES, lpEventAttributes, BOOL, bManualReset, BOOL, bInitialState, LPCSTR, lpName);
MOCKABLE_FUNCTION(WINAPI, BOOL, mocked_CloseHandle, HANDLE, hObject);
MOCKABLE_FUNCTION(WINAPI, BOOL, mocked_SetEvent, HANDLE, hEvent);
MOCKABLE_FUNCTION(WINAPI, BOOL, mocked_ResetEvent, HANDLE, hEvent);
MOCKABLE_FUNCTION(WINAPI, DWORD, mocked_WaitForMultipleObjects, DWORD, nCount, const HANDLE*, lpHandles, BOOL, bWaitAll, DWORD, dwMilliseconds);
MOCKABLE_FUNCTION(WINAPI, DWORD, mocked_WaitForSingleObject, HANDLE, hHandle, DWORD, dwMilliseconds);
MOCKABLE_FUNCTION(WINAPI, BOOL, mocked_ReleaseMutex, HANDLE, hMutex);
MOCKABLE_FUNCTION(WINAPI, DWORD, mocked_GetCurrentThreadId);

MOCKABLE_FUNCTION(, void, test_on_acquire_complete, void*, context, ROBUST_SEMAPHORE_ACQUIRE_RESULT, result, uint32_t, unit_index);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#define TEST_NAME "Global\\test_semaphore"
#define TEST_MAX_COUNT 2
#define TEST_SECURITY_DESCRIPTOR "D:P(A;;GA;;;SY)(A;;GA;;;BA)"
#define TEST_TIMEOUT_MS 10000
#define TEST_MAX_HOLD_MS 5000
#define TEST_WORKER_THREAD_ID 1111
#define TEST_OTHER_THREAD_ID 2222

static PSECURITY_DESCRIPTOR test_security_descriptor = (PSECURITY_DESCRIPTOR)0x4242;
static HANDLE test_cancel_event = (HANDLE)0x5001;
static HANDLE test_release_event = (HANDLE)0x5002;
static HANDLE test_mutexes[ROBUST_SEMAPHORE_MAX_COUNT];
static THREAD_HANDLE test_worker_thread = (THREAD_HANDLE)0x7001;
static void* test_context = (void*)0x8001;

static char expected_mutex_names[ROBUST_SEMAPHORE_MAX_COUNT][64];

static THREAD_START_FUNC captured_worker_thread_func;
static void* captured_worker_thread_arg;
static DWORD current_thread_id;

/* when set, ThreadAPI_Join runs the worker thread function, like a real worker thread that ends while robust_semaphore_destroy waits for it */
static bool run_worker_thread_in_join;

static int acquire_async_result_from_callback;

/* optionally called from on_acquire_complete, to call the API from the callback like a real user would */
typedef void (*ON_ACQUIRE_COMPLETE_ACTION)(ROBUST_SEMAPHORE_HANDLE robust_semaphore);
static ON_ACQUIRE_COMPLETE_ACTION on_acquire_complete_action;
static ROBUST_SEMAPHORE_HANDLE on_acquire_complete_action_robust_semaphore;

TEST_DEFINE_ENUM_TYPE(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static BOOL WINAPI hook_ConvertStringSecurityDescriptorToSecurityDescriptorA(LPCSTR StringSecurityDescriptor, DWORD StringSDRevision, PSECURITY_DESCRIPTOR* SecurityDescriptor, PULONG SecurityDescriptorSize)
{
    (void)StringSecurityDescriptor;
    (void)StringSDRevision;
    (void)SecurityDescriptorSize;
    *SecurityDescriptor = test_security_descriptor;
    return TRUE;
}

static THREADAPI_RESULT hook_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    captured_worker_thread_func = func;
    captured_worker_thread_arg = arg;
    *threadHandle = test_worker_thread;
    return THREADAPI_OK;
}

static DWORD WINAPI hook_GetCurrentThreadId(void)
{
    return current_thread_id;
}

static THREADAPI_RESULT hook_ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res)
{
    (void)threadHandle;
    int thread_result = 0;
    if (!run_worker_thread_in_join)
    {
        /* do nothing, the worker thread already ended */
    }
    else
    {
        DWORD saved_thread_id = current_thread_id;
        current_thread_id = TEST_WORKER_THREAD_ID;
        thread_result = captured_worker_thread_func(captured_worker_thread_arg);
        current_thread_id = saved_thread_id;
    }

    if (res == NULL)
    {
        /* do nothing, the caller does not want the thread result */
    }
    else
    {
        *res = thread_result;
    }
    return THREADAPI_OK;
}

static void hook_test_on_acquire_complete(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    (void)context;
    (void)result;
    (void)unit_index;
    if (on_acquire_complete_action == NULL)
    {
        /* do nothing, the test does not call the API from the callback */
    }
    else
    {
        ON_ACQUIRE_COMPLETE_ACTION action = on_acquire_complete_action;
        on_acquire_complete_action = NULL;
        action(on_acquire_complete_action_robust_semaphore);
    }
}

static void expect_create(const char* security_descriptor)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    if (security_descriptor == NULL)
    {
        /* do nothing, no security descriptor to convert */
    }
    else
    {
        STRICT_EXPECTED_CALL(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA(security_descriptor, SDDL_REVISION_1, IGNORED_ARG, NULL));
    }
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[i], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
            .SetReturn(test_mutexes[i]);
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    if (security_descriptor == NULL)
    {
        /* do nothing, no security descriptor to free */
    }
    else
    {
        STRICT_EXPECTED_CALL(mocked_LocalFree(test_security_descriptor))
            .CallCannotFail();
    }
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL))
        .SetReturn(test_cancel_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL))
        .SetReturn(test_release_event);
}

static ROBUST_SEMAPHORE_HANDLE test_create(void)
{
    expect_create(TEST_SECURITY_DESCRIPTOR);
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
}

static void expect_free(void)
{
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(mocked_CloseHandle(test_mutexes[i]));
    }
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_release_event));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_cancel_event));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

/* destroy from a thread other than the worker thread, when no worker thread is running and there is no acquisition to release */
static void expect_destroy(bool worker_thread_started)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    if (!worker_thread_started)
    {
        /* do nothing, there is no worker thread to wait for */
    }
    else
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG));
    }
    expect_free();
}

/* destroy from a thread other than the worker thread, a running worker thread ends while robust_semaphore_destroy waits for it */
static void test_destroy(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    current_thread_id = TEST_OTHER_THREAD_ID;
    run_worker_thread_in_join = true;
    umock_c_reset_all_calls();
    robust_semaphore_destroy(robust_semaphore);
    umock_c_reset_all_calls();
    run_worker_thread_in_join = false;
}

static void test_acquire_async(ROBUST_SEMAPHORE_HANDLE robust_semaphore, uint32_t timeout_ms, uint32_t max_hold_ms)
{
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, timeout_ms, max_hold_ms, test_on_acquire_complete, test_context));
    umock_c_reset_all_calls();
}

/* robust_semaphore_acquire_async when no worker thread is running */
static void expect_acquire_async_starts_worker_thread(ROBUST_SEMAPHORE_HANDLE robust_semaphore, bool previous_worker_thread)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    if (!previous_worker_thread)
    {
        /* do nothing, there is no previous worker thread to wait for */
    }
    else
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, robust_semaphore));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
}

/* robust_semaphore_acquire_async when the worker thread is running */
static void expect_acquire_async_leaves_the_acquisition_to_the_worker_thread(void)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
}

static void expect_worker_start(void)
{
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
}

static void expect_worker_takes_acquisition(void)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_ResetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_ResetEvent(test_release_event));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
}

static void expect_wait_for_unit(uint32_t timeout_ms, DWORD wait_result)
{
    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(1 + TEST_MAX_COUNT, IGNORED_ARG, FALSE, timeout_ms))
        .SetReturn(wait_result);
}

static void expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_RESULT acquire_result)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, acquire_result, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
}

/* the unit is acquired at 1000 ms; when there is a hold limit, on_acquire_complete returns at callback_returned_ms */
static void expect_hold_unit(uint32_t unit_index, uint32_t max_hold_ms, double callback_returned_ms, DWORD expected_hold_timeout_ms, DWORD hold_wait_result)
{
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000.0);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_OK, unit_index));
    if (max_hold_ms == ROBUST_SEMAPHORE_NO_TIMEOUT)
    {
        /* do nothing, no time left to compute */
    }
    else
    {
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
            .SetReturn(callback_returned_ms);
    }
    STRICT_EXPECTED_CALL(mocked_WaitForSingleObject(test_release_event, expected_hold_timeout_ms))
        .SetReturn(hold_wait_result);
    STRICT_EXPECTED_CALL(mocked_ReleaseMutex(test_mutexes[unit_index]));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
}

/* the worker thread finds no new acquisition and ends */
static void expect_worker_ends(void)
{
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
}

static void run_worker_thread(void)
{
    ASSERT_IS_NOT_NULL(captured_worker_thread_func);
    current_thread_id = TEST_WORKER_THREAD_ID;
    ASSERT_ARE_EQUAL(int, 0, captured_worker_thread_func(captured_worker_thread_arg));
    current_thread_id = TEST_OTHER_THREAD_ID;
}

static void call_robust_semaphore_acquire_async(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT, test_on_acquire_complete, test_context));
}

static void call_robust_semaphore_release(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    robust_semaphore_release(robust_semaphore);
}

static void call_robust_semaphore_destroy(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    robust_semaphore_destroy(robust_semaphore);
}

static void call_robust_semaphore_release_then_acquire_async(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    robust_semaphore_release(robust_semaphore);
    acquire_async_result_from_callback = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT, test_on_acquire_complete, test_context);
}

static void call_robust_semaphore_destroy_then_acquire_async(ROBUST_SEMAPHORE_HANDLE robust_semaphore)
{
    robust_semaphore_destroy(robust_semaphore);
    acquire_async_result_from_callback = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT, test_on_acquire_complete, test_context);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_ll_init, MU_FAILURE);

    REGISTER_UMOCK_ALIAS_TYPE(PSECURITY_DESCRIPTOR*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PULONG, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HLOCAL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const HANDLE*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_LL*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(int*, void*);
    REGISTER_TYPE(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT);
    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA, hook_ConvertStringSecurityDescriptorToSecurityDescriptorA);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_LocalFree, NULL, test_security_descriptor);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_CreateMutexExA, (HANDLE)0x6000, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_CreateEventA, test_cancel_event, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_CloseHandle, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_SetEvent, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_ResetEvent, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_WaitForMultipleObjects, WAIT_OBJECT_0, WAIT_FAILED);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_WaitForSingleObject, WAIT_OBJECT_0, WAIT_FAILED);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_ReleaseMutex, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_GetCurrentThreadId, hook_GetCurrentThreadId);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, hook_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, hook_ThreadAPI_Join);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(timer_global_get_elapsed_ms, 1000.0);
    REGISTER_GLOBAL_MOCK_HOOK(test_on_acquire_complete, hook_test_on_acquire_complete);

    for (uint32_t i = 0; i < ROBUST_SEMAPHORE_MAX_COUNT; i++)
    {
        test_mutexes[i] = (HANDLE)(uintptr_t)(0x6000 + i);
        ASSERT_IS_TRUE(snprintf(expected_mutex_names[i], sizeof(expected_mutex_names[i]), "%s_%" PRIu32 "", TEST_NAME, i) > 0);
    }
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    captured_worker_thread_func = NULL;
    captured_worker_thread_arg = NULL;
    current_thread_id = TEST_OTHER_THREAD_ID;
    run_worker_thread_in_join = false;
    acquire_async_result_from_callback = 0;
    on_acquire_complete_action = NULL;
    on_acquire_complete_action_robust_semaphore = NULL;

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// robust_semaphore_create
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_001: [ If name is NULL then robust_semaphore_create shall fail and return NULL. ]*/
TEST_FUNCTION(robust_semaphore_create_with_NULL_name_fails)
{
    // arrange

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(NULL, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_002: [ If max_count is 0 then robust_semaphore_create shall fail and return NULL. ]*/
TEST_FUNCTION(robust_semaphore_create_with_0_max_count_fails)
{
    // arrange

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, 0, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_003: [ If max_count is greater than ROBUST_SEMAPHORE_MAX_COUNT then robust_semaphore_create shall fail and return NULL. ]*/
TEST_FUNCTION(robust_semaphore_create_with_max_count_above_the_limit_fails)
{
    // arrange

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, ROBUST_SEMAPHORE_MAX_COUNT + 1, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_004: [ robust_semaphore_create shall allocate memory for the robust semaphore object. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_005: [ robust_semaphore_create shall initialize a lock by calling srw_lock_ll_init. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_006: [ If security_descriptor is not NULL, robust_semaphore_create shall convert it by calling ConvertStringSecurityDescriptorToSecurityDescriptorA. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_007: [ build the mutex name <name>_<i> by calling sprintf_char. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_008: [ create or open the mutex by calling CreateMutexExA with the converted security descriptor, the mutex name and SYNCHRONIZE | MUTEX_MODIFY_STATE access. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_009: [ robust_semaphore_create shall free the converted security descriptor by calling LocalFree. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_012: [ robust_semaphore_create shall succeed and return the robust semaphore object. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_054: [ For each unit i, robust_semaphore_create shall: ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_055: [ free the mutex name. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_056: [ robust_semaphore_create shall create a manual reset cancel event by calling CreateEventA. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_057: [ robust_semaphore_create shall create a manual reset release event by calling CreateEventA. ]*/
TEST_FUNCTION(robust_semaphore_create_succeeds_without_starting_a_worker_thread)
{
    // arrange
    expect_create(TEST_SECURITY_DESCRIPTOR);

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(captured_worker_thread_func);

    // cleanup
    test_destroy(result);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_006: [ If security_descriptor is not NULL, robust_semaphore_create shall convert it by calling ConvertStringSecurityDescriptorToSecurityDescriptorA. ]*/
TEST_FUNCTION(robust_semaphore_create_with_NULL_security_descriptor_succeeds)
{
    // arrange
    expect_create(NULL);

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, NULL);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(result);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_007: [ build the mutex name <name>_<i> by calling sprintf_char. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_008: [ create or open the mutex by calling CreateMutexExA with the converted security descriptor, the mutex name and SYNCHRONIZE | MUTEX_MODIFY_STATE access. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_054: [ For each unit i, robust_semaphore_create shall: ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_055: [ free the mutex name. ]*/
TEST_FUNCTION(robust_semaphore_create_with_ROBUST_SEMAPHORE_MAX_COUNT_creates_all_the_mutexes)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    for (uint32_t i = 0; i < ROBUST_SEMAPHORE_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[i], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
            .SetReturn(test_mutexes[i]);
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL));

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, ROBUST_SEMAPHORE_MAX_COUNT, NULL);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(result);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_009: [ robust_semaphore_create shall free the converted security descriptor by calling LocalFree. ]*/
TEST_FUNCTION(robust_semaphore_create_succeeds_when_LocalFree_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA(TEST_SECURITY_DESCRIPTOR, SDDL_REVISION_1, IGNORED_ARG, NULL));
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[i], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
            .SetReturn(test_mutexes[i]);
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(mocked_LocalFree(test_security_descriptor))
        .SetReturn(test_security_descriptor);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL));

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(result);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_robust_semaphore_create_fails)
{
    // arrange
    expect_create(TEST_SECURITY_DESCRIPTOR);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
        else
        {
            /* do nothing, this call cannot fail */
        }
    }
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
TEST_FUNCTION(when_creating_the_release_event_fails_robust_semaphore_create_closes_all_the_handles_even_when_CloseHandle_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA(TEST_SECURITY_DESCRIPTOR, SDDL_REVISION_1, IGNORED_ARG, NULL));
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[i], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
            .SetReturn(test_mutexes[i]);
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(mocked_LocalFree(test_security_descriptor));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL))
        .SetReturn(test_cancel_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, TRUE, FALSE, NULL))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_cancel_event))
        .SetReturn(FALSE);
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(mocked_CloseHandle(test_mutexes[i]))
            .SetReturn(FALSE);
    }
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_013: [ If there are any failures then robust_semaphore_create shall close the handles it created by calling CloseHandle, free the memory it allocated and return NULL. ]*/
TEST_FUNCTION(when_CreateMutexExA_fails_for_the_second_unit_robust_semaphore_create_closes_the_first_mutex)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA(TEST_SECURITY_DESCRIPTOR, SDDL_REVISION_1, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[0], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
        .SetReturn(test_mutexes[0]);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateMutexExA(IGNORED_ARG, expected_mutex_names[1], 0, SYNCHRONIZE | MUTEX_MODIFY_STATE))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_LocalFree(test_security_descriptor));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_mutexes[0]));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    ROBUST_SEMAPHORE_HANDLE result = robust_semaphore_create(TEST_NAME, TEST_MAX_COUNT, TEST_SECURITY_DESCRIPTOR);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// robust_semaphore_destroy
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_014: [ If robust_semaphore is NULL then robust_semaphore_destroy shall return. ]*/
TEST_FUNCTION(robust_semaphore_destroy_with_NULL_returns)
{
    // arrange

    // act
    robust_semaphore_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_015: [ robust_semaphore_destroy shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_061: [ robust_semaphore_destroy shall mark the object as being destroyed and release the current acquisition as robust_semaphore_release does. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_062: [ Otherwise robust_semaphore_destroy shall release the lock and, if a worker thread was started, wait for it to end by calling ThreadAPI_Join. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_019: [ robust_semaphore_destroy shall close the mutexes and the events by calling CloseHandle. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_058: [ robust_semaphore_destroy shall deinitialize the lock by calling srw_lock_ll_deinit. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_059: [ robust_semaphore_destroy shall free the robust semaphore object. ]*/
TEST_FUNCTION(robust_semaphore_destroy_without_a_worker_thread_frees_the_object)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    expect_destroy(false);

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_062: [ Otherwise robust_semaphore_destroy shall release the lock and, if a worker thread was started, wait for it to end by calling ThreadAPI_Join. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_019: [ robust_semaphore_destroy shall close the mutexes and the events by calling CloseHandle. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_058: [ robust_semaphore_destroy shall deinitialize the lock by calling srw_lock_ll_deinit. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_059: [ robust_semaphore_destroy shall free the robust semaphore object. ]*/
TEST_FUNCTION(robust_semaphore_destroy_waits_for_the_worker_thread_that_ended_and_frees_the_object)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    run_worker_thread();
    umock_c_reset_all_calls();
    expect_destroy(true);

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_061: [ robust_semaphore_destroy shall mark the object as being destroyed and release the current acquisition as robust_semaphore_release does. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_062: [ Otherwise robust_semaphore_destroy shall release the lock and, if a worker thread was started, wait for it to end by calling ThreadAPI_Join. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_070: [ If there is no new acquisition, robust_semaphore_worker_thread shall, while still holding the lock, mark the worker thread as not running. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_071: [ robust_semaphore_worker_thread shall then release the lock and return 0 without taking the lock again. ]*/
TEST_FUNCTION(robust_semaphore_destroy_cancels_a_pending_acquisition_and_waits_for_the_worker_thread_to_complete_it_and_end)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    run_worker_thread_in_join = true;

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event));
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG));
    /* the worker thread completes the released acquisition with CANCELLED */
    expect_worker_start();
    expect_worker_takes_acquisition();
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    /* and ends because there is no new acquisition */
    expect_worker_ends();
    expect_free();

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_053: [ If ThreadAPI_Join fails, robust_semaphore_destroy shall return without freeing the object, because the worker thread may still use it. ]*/
TEST_FUNCTION(when_ThreadAPI_Join_fails_robust_semaphore_destroy_does_not_free_the_object)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event));
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    /* destroying again waits for the worker thread and frees the object */
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_019: [ robust_semaphore_destroy shall close the mutexes and the events by calling CloseHandle. ]*/
TEST_FUNCTION(robust_semaphore_destroy_frees_the_object_even_when_CloseHandle_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    for (uint32_t i = 0; i < TEST_MAX_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(mocked_CloseHandle(test_mutexes[i]))
            .SetReturn(FALSE);
    }
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_release_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_cancel_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_017: [ If robust_semaphore_destroy is called on the worker thread, it shall mark the worker thread as the one that frees the object, release the lock and return. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_049: [ If robust_semaphore_destroy was called on the worker thread, before returning robust_semaphore_worker_thread shall close its own thread handle by calling CloseHandle. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_060: [ If robust_semaphore_destroy was called on the worker thread, before returning robust_semaphore_worker_thread shall free the robust semaphore object as robust_semaphore_destroy does. ]*/
TEST_FUNCTION(robust_semaphore_destroy_called_from_on_acquire_complete_lets_the_worker_thread_free_the_object)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    on_acquire_complete_action = call_robust_semaphore_destroy;
    on_acquire_complete_action_robust_semaphore = robust_semaphore;

    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_TIMEOUT);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    /* destroy, called from the callback on the worker thread */
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    /* the worker thread finds no new acquisition, ends and frees the object */
    expect_worker_ends();
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_worker_thread));
    expect_free();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// robust_semaphore_acquire_async
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_020: [ If robust_semaphore is NULL then robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_with_NULL_robust_semaphore_fails)
{
    // arrange

    // act
    int result = robust_semaphore_acquire_async(NULL, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_021: [ If on_acquire_complete is NULL then robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_with_NULL_on_acquire_complete_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, NULL, test_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_022: [ robust_semaphore_acquire_async shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_064: [ robust_semaphore_acquire_async shall store timeout_ms, max_hold_ms, on_acquire_complete and on_acquire_complete_context as the new current acquisition. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_066: [ Otherwise robust_semaphore_acquire_async shall: ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_068: [ start a new worker thread by calling ThreadAPI_Create with robust_semaphore_worker_thread and mark it as running. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_026: [ robust_semaphore_acquire_async shall release the lock and succeed and return 0. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_starts_a_worker_thread_and_succeeds)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    expect_acquire_async_starts_worker_thread(robust_semaphore, false);

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(captured_worker_thread_func);
    ASSERT_ARE_EQUAL(void_ptr, robust_semaphore, captured_worker_thread_arg);

    /* the worker thread runs the stored acquisition */
    umock_c_reset_all_calls();
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_TIMEOUT);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT);
    expect_worker_ends();
    run_worker_thread();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_066: [ Otherwise robust_semaphore_acquire_async shall: ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_067: [ if a worker thread was started before, wait for it to end by calling ThreadAPI_Join. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_068: [ start a new worker thread by calling ThreadAPI_Create with robust_semaphore_worker_thread and mark it as running. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_026: [ robust_semaphore_acquire_async shall release the lock and succeed and return 0. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_after_the_worker_thread_ended_waits_for_it_and_starts_a_new_worker_thread)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    run_worker_thread();
    umock_c_reset_all_calls();
    captured_worker_thread_func = NULL;
    expect_acquire_async_starts_worker_thread(robust_semaphore, true);

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(captured_worker_thread_func);

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_024: [ If there is a current acquisition that was not released or ended, robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_while_an_acquisition_is_in_progress_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_069: [ If ThreadAPI_Join or ThreadAPI_Create fails, robust_semaphore_acquire_async shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. ]*/
TEST_FUNCTION(when_ThreadAPI_Create_fails_robust_semaphore_acquire_async_fails_and_a_new_acquisition_can_be_started)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, robust_semaphore))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /* there is no worker thread to wait for */
    umock_c_reset_all_calls();
    expect_acquire_async_starts_worker_thread(robust_semaphore, false);
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_069: [ If ThreadAPI_Join or ThreadAPI_Create fails, robust_semaphore_acquire_async shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. ]*/
TEST_FUNCTION(when_ThreadAPI_Join_of_the_previous_worker_thread_fails_robust_semaphore_acquire_async_fails_and_a_new_acquisition_can_be_started)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    run_worker_thread();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    int result = robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /* the previous worker thread is not waited for again */
    umock_c_reset_all_calls();
    expect_acquire_async_starts_worker_thread(robust_semaphore, false);
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_063: [ If the object is being destroyed, robust_semaphore_acquire_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(robust_semaphore_acquire_async_while_the_object_is_being_destroyed_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, 0, ROBUST_SEMAPHORE_NO_TIMEOUT);
    /* destroy on the worker thread marks the object as being destroyed without freeing it */
    on_acquire_complete_action = call_robust_semaphore_destroy_then_acquire_async;
    on_acquire_complete_action_robust_semaphore = robust_semaphore;
    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(WAIT_TIMEOUT);

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, acquire_async_result_from_callback);
}

//
// robust_semaphore_cancel
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_028: [ If robust_semaphore is NULL then robust_semaphore_cancel shall return. ]*/
TEST_FUNCTION(robust_semaphore_cancel_with_NULL_returns)
{
    // arrange

    // act
    robust_semaphore_cancel(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_029: [ robust_semaphore_cancel shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_030: [ If the current acquisition was not released and did not get a unit, robust_semaphore_cancel shall mark it as cancelled and signal the cancel event by calling SetEvent. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_031: [ robust_semaphore_cancel shall release the lock. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_038: [ If the acquisition was already cancelled or released, robust_semaphore_worker_thread shall end it and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED and ROBUST_SEMAPHORE_UNIT_INDEX_NONE without waiting for a unit. ]*/
TEST_FUNCTION(robust_semaphore_cancel_of_an_acquisition_not_taken_yet_completes_it_with_CANCELLED_without_waiting)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_cancel(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();
    expect_worker_start();
    expect_worker_takes_acquisition();
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    expect_worker_ends();
    run_worker_thread();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_030: [ If the current acquisition was not released and did not get a unit, robust_semaphore_cancel shall mark it as cancelled and signal the cancel event by calling SetEvent. ]*/
TEST_FUNCTION(robust_semaphore_cancel_without_an_acquisition_does_nothing)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_cancel(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_030: [ If the current acquisition was not released and did not get a unit, robust_semaphore_cancel shall mark it as cancelled and signal the cancel event by calling SetEvent. ]*/
TEST_FUNCTION(robust_semaphore_cancel_when_SetEvent_fails_still_cancels)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_cancel(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /* the acquisition is still marked as cancelled */
    umock_c_reset_all_calls();
    expect_worker_start();
    expect_worker_takes_acquisition();
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    expect_worker_ends();
    run_worker_thread();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

//
// robust_semaphore_release
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_032: [ If robust_semaphore is NULL then robust_semaphore_release shall return. ]*/
TEST_FUNCTION(robust_semaphore_release_with_NULL_returns)
{
    // arrange

    // act
    robust_semaphore_release(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_033: [ robust_semaphore_release shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_034: [ If there is a current acquisition that was not released, robust_semaphore_release shall mark it as released and signal the cancel event and the release event by calling SetEvent. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_035: [ robust_semaphore_release shall release the lock. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_038: [ If the acquisition was already cancelled or released, robust_semaphore_worker_thread shall end it and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED and ROBUST_SEMAPHORE_UNIT_INDEX_NONE without waiting for a unit. ]*/
TEST_FUNCTION(robust_semaphore_release_of_an_acquisition_not_taken_yet_completes_it_with_CANCELLED)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_release(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /* the waiting acquisition ends only when the worker thread completes it */
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    ASSERT_ARE_NOT_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();
    expect_worker_start();
    expect_worker_takes_acquisition();
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    expect_worker_ends();
    run_worker_thread();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_034: [ If there is a current acquisition that was not released, robust_semaphore_release shall mark it as released and signal the cancel event and the release event by calling SetEvent. ]*/
TEST_FUNCTION(robust_semaphore_release_twice_signals_the_events_once)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    robust_semaphore_release(robust_semaphore);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_release(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_034: [ If there is a current acquisition that was not released, robust_semaphore_release shall mark it as released and signal the cancel event and the release event by calling SetEvent. ]*/
TEST_FUNCTION(robust_semaphore_release_without_an_acquisition_does_nothing)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));

    // act
    robust_semaphore_release(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_051: [ If the current acquisition holds a unit, robust_semaphore_release shall end it. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_047: [ If a unit was acquired, robust_semaphore_worker_thread shall release the mutex by calling ReleaseMutex and, if the acquisition still holds the unit (it was not released), end it. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_065: [ If the worker thread is running, robust_semaphore_acquire_async shall leave the new acquisition to it. ]*/
TEST_FUNCTION(releasing_a_held_unit_from_on_acquire_complete_ends_the_acquisition_and_a_new_one_can_start_right_away)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    on_acquire_complete_action = call_robust_semaphore_release_then_acquire_async;
    on_acquire_complete_action_robust_semaphore = robust_semaphore;

    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_OK, 0));
    /* release, then acquire again, from the callback: the worker thread is running, so no new worker thread is started */
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    expect_acquire_async_leaves_the_acquisition_to_the_worker_thread();
    /* the worker thread releases the first unit without touching the new acquisition */
    STRICT_EXPECTED_CALL(mocked_WaitForSingleObject(test_release_event, INFINITE));
    STRICT_EXPECTED_CALL(mocked_ReleaseMutex(test_mutexes[0]));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    /* and then takes the new acquisition */
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_TIMEOUT);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, acquire_async_result_from_callback);

    // cleanup
    test_destroy(robust_semaphore);
}

//
// robust_semaphore_worker_thread
//

/*Tests_SRS_ROBUST_SEMAPHORE_01_036: [ robust_semaphore_worker_thread shall record the id of the worker thread by calling GetCurrentThreadId. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_037: [ If there is a new acquisition, robust_semaphore_worker_thread shall take it and reset the cancel event and the release event by calling ResetEvent. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_039: [ Otherwise robust_semaphore_worker_thread shall wait for the cancel event or any of the mutexes by calling WaitForMultipleObjects with the cancel event followed by the max_count mutexes, bWaitAll FALSE and timeout_ms as timeout (INFINITE if timeout_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_041: [ If WaitForMultipleObjects returns WAIT_OBJECT_0 + 1 + i or WAIT_ABANDONED_0 + 1 + i then robust_semaphore_worker_thread shall record the time the unit was acquired by calling timer_global_get_elapsed_ms, mark the acquisition as holding unit i and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_OK and i. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_045: [ If a unit was acquired, after on_acquire_complete returns, robust_semaphore_worker_thread shall wait for the release event by calling WaitForSingleObject with what is left of max_hold_ms since the unit was acquired (measured by calling timer_global_get_elapsed_ms, 0 if max_hold_ms already passed, INFINITE if max_hold_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_047: [ If a unit was acquired, robust_semaphore_worker_thread shall release the mutex by calling ReleaseMutex and, if the acquisition still holds the unit (it was not released), end it. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_070: [ If there is no new acquisition, robust_semaphore_worker_thread shall, while still holding the lock, mark the worker thread as not running. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_071: [ robust_semaphore_worker_thread shall then release the lock and return 0 without taking the lock again. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_acquires_unit_1_holds_it_until_released_and_ends)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(INFINITE, WAIT_OBJECT_0 + 2);
    expect_hold_unit(1, ROBUST_SEMAPHORE_NO_TIMEOUT, 0, INFINITE, WAIT_OBJECT_0);
    /* there is no new acquisition: the worker thread ends */
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_041: [ If WaitForMultipleObjects returns WAIT_OBJECT_0 + 1 + i or WAIT_ABANDONED_0 + 1 + i then robust_semaphore_worker_thread shall record the time the unit was acquired by calling timer_global_get_elapsed_ms, mark the acquisition as holding unit i and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_OK and i. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_acquires_an_abandoned_unit)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_ABANDONED_0 + 2);
    expect_hold_unit(1, ROBUST_SEMAPHORE_NO_TIMEOUT, 0, INFINITE, WAIT_OBJECT_0);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_037: [ If there is a new acquisition, robust_semaphore_worker_thread shall take it and reset the cancel event and the release event by calling ResetEvent. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_waits_for_a_unit_even_when_ResetEvent_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    expect_worker_start();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_ResetEvent(test_cancel_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_ResetEvent(test_release_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    expect_hold_unit(0, ROBUST_SEMAPHORE_NO_TIMEOUT, 0, INFINITE, WAIT_OBJECT_0);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_040: [ If WaitForMultipleObjects returns WAIT_OBJECT_0 then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_CANCELLED and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_completes_with_CANCELLED_when_the_cancel_event_is_signaled)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_CANCELLED);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_042: [ If WaitForMultipleObjects returns WAIT_TIMEOUT then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_completes_with_TIMEOUT_when_no_unit_is_free_within_timeout_ms)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, 0, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(0, WAIT_TIMEOUT);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_043: [ If WaitForMultipleObjects returns any other value then robust_semaphore_worker_thread shall end the acquisition and call on_acquire_complete with ROBUST_SEMAPHORE_ACQUIRE_ERROR and ROBUST_SEMAPHORE_UNIT_INDEX_NONE. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_completes_with_ERROR_when_WaitForMultipleObjects_fails)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_FAILED);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_ERROR);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_044: [ robust_semaphore_worker_thread shall end an acquisition that did not get a unit before calling its on_acquire_complete, so that on_acquire_complete can start a new acquisition. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_065: [ If the worker thread is running, robust_semaphore_acquire_async shall leave the new acquisition to it. ]*/
TEST_FUNCTION(on_acquire_complete_of_an_acquisition_that_timed_out_can_start_a_new_acquisition_that_the_same_worker_thread_takes)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    on_acquire_complete_action = call_robust_semaphore_acquire_async;
    on_acquire_complete_action_robust_semaphore = robust_semaphore;
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_TIMEOUT);
    expect_end_without_unit(ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT);
    /* the new acquisition, started from the callback, does not start a new worker thread */
    expect_acquire_async_leaves_the_acquisition_to_the_worker_thread();
    /* and is taken right away */
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    expect_hold_unit(0, ROBUST_SEMAPHORE_NO_TIMEOUT, 0, INFINITE, WAIT_OBJECT_0);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_045: [ If a unit was acquired, after on_acquire_complete returns, robust_semaphore_worker_thread shall wait for the release event by calling WaitForSingleObject with what is left of max_hold_ms since the unit was acquired (measured by calling timer_global_get_elapsed_ms, 0 if max_hold_ms already passed, INFINITE if max_hold_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_holds_the_unit_for_what_is_left_of_max_hold_ms_after_on_acquire_complete_returns)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    /* acquired at 1000 ms, on_acquire_complete returns at 3000 ms, 5000 - 2000 = 3000 ms left */
    expect_hold_unit(0, TEST_MAX_HOLD_MS, 3000.0, 3000, WAIT_OBJECT_0);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_045: [ If a unit was acquired, after on_acquire_complete returns, robust_semaphore_worker_thread shall wait for the release event by calling WaitForSingleObject with what is left of max_hold_ms since the unit was acquired (measured by calling timer_global_get_elapsed_ms, 0 if max_hold_ms already passed, INFINITE if max_hold_ms is ROBUST_SEMAPHORE_NO_TIMEOUT). ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_046: [ If waiting for the release event times out then robust_semaphore_worker_thread shall release the mutex early by calling ReleaseMutex. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_releases_the_unit_right_away_when_on_acquire_complete_runs_longer_than_max_hold_ms)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    /* acquired at 1000 ms, on_acquire_complete returns at 7000 ms, max_hold_ms already passed */
    expect_hold_unit(0, TEST_MAX_HOLD_MS, 7000.0, 0, WAIT_TIMEOUT);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_046: [ If waiting for the release event times out then robust_semaphore_worker_thread shall release the mutex early by calling ReleaseMutex. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_047: [ If a unit was acquired, robust_semaphore_worker_thread shall release the mutex by calling ReleaseMutex and, if the acquisition still holds the unit (it was not released), end it. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_releases_the_unit_early_when_max_hold_ms_expires_and_a_new_acquisition_can_start)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    expect_hold_unit(0, TEST_MAX_HOLD_MS, 1000.0, TEST_MAX_HOLD_MS, WAIT_TIMEOUT);
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    /* the acquisition ended, so a new one can be started */
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context));

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_047: [ If a unit was acquired, robust_semaphore_worker_thread shall release the mutex by calling ReleaseMutex and, if the acquisition still holds the unit (it was not released), end it. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_ends_the_acquisition_even_when_ReleaseMutex_and_the_release_wait_fail)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_OK, 0));
    STRICT_EXPECTED_CALL(mocked_WaitForSingleObject(test_release_event, INFINITE))
        .SetReturn(WAIT_FAILED);
    STRICT_EXPECTED_CALL(mocked_ReleaseMutex(test_mutexes[0]))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    expect_worker_ends();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    /* the acquisition ended, so a new one can be started */
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, TEST_MAX_HOLD_MS, test_on_acquire_complete, test_context));

    // cleanup
    test_destroy(robust_semaphore);
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_070: [ If there is no new acquisition, robust_semaphore_worker_thread shall, while still holding the lock, mark the worker thread as not running. ]*/
/*Tests_SRS_ROBUST_SEMAPHORE_01_071: [ robust_semaphore_worker_thread shall then release the lock and return 0 without taking the lock again. ]*/
TEST_FUNCTION(robust_semaphore_worker_thread_releases_a_held_unit_and_ends_when_the_object_is_destroyed)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    /* destroyed while the unit is held: from on_acquire_complete, the only place where it can happen when the worker thread runs on the test thread */
    on_acquire_complete_action = call_robust_semaphore_destroy;
    on_acquire_complete_action_robust_semaphore = robust_semaphore;

    expect_worker_start();
    expect_worker_takes_acquisition();
    expect_wait_for_unit(TEST_TIMEOUT_MS, WAIT_OBJECT_0 + 1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_OK, 0));
    /* destroy releases the held unit */
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event));
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    /* the worker thread releases the mutex */
    STRICT_EXPECTED_CALL(mocked_WaitForSingleObject(test_release_event, INFINITE));
    STRICT_EXPECTED_CALL(mocked_ReleaseMutex(test_mutexes[0]));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    /* finds no new acquisition: it ends and frees the object */
    expect_worker_ends();
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_worker_thread));
    expect_free();

    // act
    run_worker_thread();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ROBUST_SEMAPHORE_01_070: [ If there is no new acquisition, robust_semaphore_worker_thread shall, while still holding the lock, mark the worker thread as not running. ]*/
TEST_FUNCTION(robust_semaphore_destroy_on_a_thread_that_reuses_the_id_of_an_ended_worker_thread_waits_for_the_new_worker_thread)
{
    // arrange
    ROBUST_SEMAPHORE_HANDLE robust_semaphore = test_create();
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    run_worker_thread();
    /* the new worker thread did not record its id yet */
    test_acquire_async(robust_semaphore, TEST_TIMEOUT_MS, ROBUST_SEMAPHORE_NO_TIMEOUT);
    /* the id of the worker thread that ended is reused by the thread calling robust_semaphore_destroy */
    current_thread_id = TEST_WORKER_THREAD_ID;
    run_worker_thread_in_join = true;

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_cancel_event));
    STRICT_EXPECTED_CALL(mocked_SetEvent(test_release_event));
    STRICT_EXPECTED_CALL(mocked_GetCurrentThreadId());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_worker_thread, IGNORED_ARG));
    expect_worker_start();
    expect_worker_takes_acquisition();
    STRICT_EXPECTED_CALL(test_on_acquire_complete(test_context, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, ROBUST_SEMAPHORE_UNIT_INDEX_NONE));
    expect_worker_ends();
    expect_free();

    // act
    robust_semaphore_destroy(robust_semaphore);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
