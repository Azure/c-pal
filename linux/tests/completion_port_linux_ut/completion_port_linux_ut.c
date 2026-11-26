// Copyright(C) Microsoft Corporation.All rights reserved.


#include "completion_port_linux_ut_pch.h"

#define TEST_SOCKET_ENTRY_BUCKET_COUNT          256
#define TEST_MAX_CAPTURED_EPOLL_CTL_CALLS       16

#define TEST_RECEIVE_EPOLL_OP                   (EPOLLIN | EPOLLRDHUP | EPOLLONESHOT)
#define TEST_NOTIFY_IN_EPOLL_OP                 EPOLLIN
#define TEST_SEND_EPOLL_OP                      EPOLLOUT

#define TEST_INBOUND_INTEREST                   (uint32_t)(EPOLLIN | EPOLLRDHUP | EPOLLONESHOT)
#define TEST_OUTBOUND_INTEREST                  (uint32_t)(EPOLLOUT | EPOLLONESHOT)
#define TEST_INBOUND_AND_OUTBOUND_INTEREST      (uint32_t)(EPOLLIN | EPOLLRDHUP | EPOLLOUT | EPOLLONESHOT)

static SOCKET_HANDLE test_socket = (SOCKET_HANDLE)0x4242;
static SOCKET_HANDLE test_socket_2 = (SOCKET_HANDLE)0x4243;
// lands in the same socket entry bucket as test_socket
static SOCKET_HANDLE test_socket_same_bucket = (SOCKET_HANDLE)(0x4242 + TEST_SOCKET_ENTRY_BUCKET_COUNT);
static void* test_callback_ctx = (void*)0x4244;
static void* test_callback_ctx_2 = (void*)0x4245;
static void* test_callback_ctx_3 = (void*)0x4246;
static THREAD_HANDLE test_thread_handle = (THREAD_HANDLE)0x4200;
static THREAD_START_FUNC g_saved_worker_thread_func;
static void* g_saved_worker_thread_func_context;

static int g_test_epoll = 2;

typedef struct CAPTURED_EPOLL_CTL_CALL_TAG
{
    int op;
    int fd;
    uint32_t events;
    void* data_ptr;
} CAPTURED_EPOLL_CTL_CALL;

static CAPTURED_EPOLL_CTL_CALL g_captured_epoll_ctl_calls[TEST_MAX_CAPTURED_EPOLL_CTL_CALLS];
static uint32_t g_captured_epoll_ctl_call_count;

// when set, the next mocked_epoll_wait call removes this socket, simulating a completion_port_remove on another thread
// that runs after epoll_wait returned and before the worker processes the returned events
static COMPLETION_PORT_HANDLE g_remove_during_epoll_wait_completion_port;
static SOCKET_HANDLE g_remove_during_epoll_wait_socket;

// when set, the next test_port_event_complete call removes this socket, simulating a completion_port_remove on another thread
// that runs while the worker executes an event callback of that socket
static COMPLETION_PORT_HANDLE g_remove_during_callback_completion_port;
static SOCKET_HANDLE g_remove_during_callback_socket;

THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    g_saved_worker_thread_func = func;
    g_saved_worker_thread_func_context = arg;
    *threadHandle = test_thread_handle;
    return THREADAPI_OK;
}

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES);

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_create, int, __size)
MOCK_FUNCTION_END(g_test_epoll)
MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_ctl, int, epfd, int, op, int, fd, struct epoll_event*, event)
    if (g_captured_epoll_ctl_call_count < TEST_MAX_CAPTURED_EPOLL_CTL_CALLS)
    {
        g_captured_epoll_ctl_calls[g_captured_epoll_ctl_call_count].op = op;
        g_captured_epoll_ctl_calls[g_captured_epoll_ctl_call_count].fd = fd;
        g_captured_epoll_ctl_calls[g_captured_epoll_ctl_call_count].events = (event == NULL) ? 0 : event->events;
        g_captured_epoll_ctl_calls[g_captured_epoll_ctl_call_count].data_ptr = (event == NULL) ? NULL : event->data.ptr;
        g_captured_epoll_ctl_call_count++;
    }
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_wait, int, epfd, struct epoll_event*, events, int, maxevents, int, timeout)
    if (g_remove_during_epoll_wait_completion_port != NULL)
    {
        COMPLETION_PORT_HANDLE completion_port = g_remove_during_epoll_wait_completion_port;
        g_remove_during_epoll_wait_completion_port = NULL;
        completion_port_remove(completion_port, g_remove_during_epoll_wait_socket);
    }
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_close, int, s)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, void, test_port_event_complete, void*, context, COMPLETION_PORT_EPOLL_ACTION, action)
    if (g_remove_during_callback_completion_port != NULL)
    {
        COMPLETION_PORT_HANDLE completion_port = g_remove_during_callback_completion_port;
        g_remove_during_callback_completion_port = NULL;
        completion_port_remove(completion_port, g_remove_during_callback_socket);
    }
MOCK_FUNCTION_END()

static void setup_completion_port_create_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();

    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_create(TEST_MAX_EVENTS_NUM));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
}

static void setup_lock_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, 1, 0))
        .CallCannotFail();
}

static void setup_unlock_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

static void setup_completion_port_add_begin_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetFailReturn(1);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    setup_lock_expectations();
}

static void setup_completion_port_add_end_expectations(void)
{
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

static void setup_completion_port_add_new_socket_expectations(SOCKET_HANDLE socket)
{
    setup_completion_port_add_begin_expectations();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_ADD, socket, IGNORED_ARG));
    setup_completion_port_add_end_expectations();
}

static void setup_completion_port_add_existing_socket_expectations(SOCKET_HANDLE socket)
{
    setup_completion_port_add_begin_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, socket, IGNORED_ARG));
    setup_completion_port_add_end_expectations();
}

static void setup_completion_port_remove_begin_expectations(SOCKET_HANDLE socket)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_DEL, socket, NULL));
    setup_unlock_expectations();
}

static void setup_completion_port_remove_retire_expectations(void)
{
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));
    setup_lock_expectations();
    setup_unlock_expectations();
}

static void setup_completion_port_remove_end_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

static void setup_completion_port_dec_ref_begin_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
}

static void setup_free_retired_socket_entries_expectations(void)
{
    setup_lock_expectations();
    setup_unlock_expectations();
}

static void setup_epoll_wait_expectations(struct epoll_event* events, int event_count)
{
    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .CopyOutArgumentBuffer_events(events, sizeof(struct epoll_event) * event_count)
        .SetReturn(event_count);
}

static void setup_worker_exit_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);
}

static void set_event(struct epoll_event* event, uint32_t events, void* socket_entry)
{
    event->events = events;
    event->data.ptr = socket_entry;
}

static COMPLETION_PORT_HANDLE create_completion_port(void)
{
    COMPLETION_PORT_HANDLE result = completion_port_create();
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

// adds an operation and returns the socket entry that completion_port_add passed as data.ptr to epoll_ctl
static void* add_operation(COMPLETION_PORT_HANDLE completion_port, int epoll_op, SOCKET_HANDLE socket, void* event_callback_ctx)
{
    uint32_t captured_call_index = g_captured_epoll_ctl_call_count;
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(completion_port, epoll_op, socket, test_port_event_complete, event_callback_ctx));
    ASSERT_ARE_EQUAL(uint32_t, captured_call_index + 1, g_captured_epoll_ctl_call_count);
    void* result = g_captured_epoll_ctl_calls[captured_call_index].data_ptr;
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static void assert_captured_epoll_ctl_call(uint32_t index, int op, SOCKET_HANDLE socket, uint32_t events, void* socket_entry)
{
    ASSERT_IS_TRUE(index < g_captured_epoll_ctl_call_count, "epoll_ctl call %" PRIu32 " was not captured", index);
    ASSERT_ARE_EQUAL(int, op, g_captured_epoll_ctl_calls[index].op);
    ASSERT_ARE_EQUAL(int, socket, g_captured_epoll_ctl_calls[index].fd);
    ASSERT_ARE_EQUAL(uint32_t, events, g_captured_epoll_ctl_calls[index].events);
    ASSERT_ARE_EQUAL(void_ptr, socket_entry, g_captured_epoll_ctl_calls[index].data_ptr);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types failed");

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_create, g_test_epoll, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_ctl, 0, -1);

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);

    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);
    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
    errno = 0;
    g_captured_epoll_ctl_call_count = 0;
    g_remove_during_epoll_wait_completion_port = NULL;
    g_remove_during_epoll_wait_socket = INVALID_SOCKET;
    g_remove_during_callback_completion_port = NULL;
    g_remove_during_callback_socket = INVALID_SOCKET;
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    umock_c_negative_tests_deinit();
}

// completion_port_create

// Tests_SRS_COMPLETION_PORT_LINUX_11_001: [ completion_port_create shall allocate memory for a completion port object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_002: [ completion_port_create shall create the epoll instance by calling epoll_create. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_003: [ completion_port_create shall create a thread that runs epoll_worker_func to handle the epoll events. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_004: [ On success completion_port_create shall return the allocated COMPLETION_PORT_HANDLE. ]
TEST_FUNCTION(completion_port_create_success)
{
    //arrange
    setup_completion_port_create_mocks();

     //act
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(port_handle);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_005: [ If there are any errors then completion_port_create shall fail and return NULL. ]
TEST_FUNCTION(completion_port_create_fails)
{
    // arrange
    COMPLETION_PORT_HANDLE port_handle;

    setup_completion_port_create_mocks();

    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            port_handle = completion_port_create();

            // assert
            ASSERT_IS_NULL(port_handle, "On failed call %zu", index);
        }
    }
}

// completion_port_inc_ref

// Tests_SRS_COMPLETION_PORT_LINUX_11_006: [ If completion_port is NULL, completion_port_inc_ref shall return. ]
TEST_FUNCTION(completion_port_inc_ref_handle_NULL_fail)
{
    //arrange

    //act
    completion_port_inc_ref(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_007: [ Otherwise completion_port_inc_ref shall increment the internally maintained reference count. ]
TEST_FUNCTION(completion_port_inc_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();

    //act
    completion_port_inc_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
    completion_port_dec_ref(port_handle);
}

// completion_port_dec_ref

// Tests_SRS_COMPLETION_PORT_LINUX_11_008: [ If completion_port is NULL, completion_port_dec_ref shall return. ]
TEST_FUNCTION(completion_port_dec_ref_handle_NULL_fail)
{
    //arrange

    //act
    completion_port_dec_ref(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_009: [ completion_port_dec_ref shall decrement the reference count for completion_port. ]
TEST_FUNCTION(completion_port_dec_ref_1_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    completion_port_inc_ref(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_010: [ If the reference count reaches 0, completion_port_dec_ref shall do the following: ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_011: [ wait for the ongoing call count to reach zero. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_012: [ increment the flag signaling that the threads can complete. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_013: [ close the epoll object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_014: [ close the thread by calling ThreadAPI_Join. ]
TEST_FUNCTION(completion_port_dec_ref_0_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_completion_port_dec_ref_begin_expectations();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_001: [ free the pending operations of every socket entry without calling their event_callback. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_002: [ free every socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_015: [ then the memory associated with completion_port shall be freed. ]
TEST_FUNCTION(when_operations_are_pending_then_completion_port_dec_ref_frees_them_without_calling_their_callbacks)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);
    void* socket_entry_same_bucket = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket_same_bucket, test_callback_ctx_3);

    setup_completion_port_dec_ref_begin_expectations();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // inbound operation of test_socket
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // outbound operation of test_socket
    STRICT_EXPECTED_CALL(free(socket_entry));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // inbound operation of test_socket_same_bucket
    STRICT_EXPECTED_CALL(free(socket_entry_same_bucket));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // completion port

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_003: [ free every retired socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_021: [ completion_port_remove shall add the socket entry to the retired list of the completion port. ]
TEST_FUNCTION(when_a_socket_was_removed_then_completion_port_dec_ref_frees_its_retired_socket_entry)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    completion_port_remove(port_handle, test_socket);
    umock_c_reset_all_calls();

    setup_completion_port_dec_ref_begin_expectations();
    STRICT_EXPECTED_CALL(free(socket_entry));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // completion port

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// completion_port_add

// Tests_SRS_COMPLETION_PORT_LINUX_11_016: [ If completion_port is NULL, completion_port_add shall return a non-zero value. ]
TEST_FUNCTION(completion_port_add_handle_NULL_fail)
{
    //arrange

    //act
    int result = completion_port_add(NULL, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_018: [ If event_callback is NULL, completion_port_add shall return a non-zero value. ]
TEST_FUNCTION(completion_port_add_event_callback_NULL_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    //act
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, NULL, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_017: [ If socket is INVALID_SOCKET, completion_port_add shall return a non-zero value. ]
TEST_FUNCTION(completion_port_add_socket_handle_INVALID_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    //act
    int result = completion_port_add(port_handle, EPOLLIN, INVALID_SOCKET, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_019: [ completion_port_add shall ensure the thread completion flag is not set. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_020: [ completion_port_add shall increment the ongoing call count value to prevent close. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_004: [ completion_port_add shall allocate a pending operation that stores epoll_op, event_callback and event_callback_ctx. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_005: [ completion_port_add shall acquire the completion port lock. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_006: [ If no socket entry exists for socket, completion_port_add shall allocate a new socket entry and add it to the socket entry lookup. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_007: [ If the socket entry is not registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_ADD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_010: [ If epoll_ctl succeeds and epoll_op does not contain EPOLLOUT, completion_port_add shall append the operation to the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_012: [ completion_port_add shall release the completion port lock. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_025: [ completion_port_add shall decrement the ongoing call count value to unblock close. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_026: [ On success, completion_port_add shall return 0. ]
TEST_FUNCTION(when_first_inbound_operation_is_added_then_completion_port_add_registers_the_socket_with_EPOLL_CTL_ADD)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_completion_port_add_new_socket_expectations(test_socket);

    //act
    int result = completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, g_captured_epoll_ctl_call_count);
    ASSERT_IS_NOT_NULL(g_captured_epoll_ctl_calls[0].data_ptr);
    assert_captured_epoll_ctl_call(0, EPOLL_CTL_ADD, test_socket, TEST_INBOUND_INTEREST, g_captured_epoll_ctl_calls[0].data_ptr);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_007: [ If the socket entry is not registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_ADD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_009: [ If epoll_ctl succeeds and epoll_op contains EPOLLOUT, completion_port_add shall append the operation to the outbound operation queue of the socket entry. ]
TEST_FUNCTION(when_first_outbound_operation_is_added_then_completion_port_add_registers_the_socket_for_EPOLLOUT)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_completion_port_add_new_socket_expectations(test_socket);

    //act
    int result = completion_port_add(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(0, EPOLL_CTL_ADD, test_socket, TEST_OUTBOUND_INTEREST, g_captured_epoll_ctl_calls[0].data_ptr);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_008: [ If the socket entry is registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_009: [ If epoll_ctl succeeds and epoll_op contains EPOLLOUT, completion_port_add shall append the operation to the outbound operation queue of the socket entry. ]
TEST_FUNCTION(when_outbound_operation_is_added_while_inbound_operation_is_pending_then_completion_port_add_keeps_both_directions_registered)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    setup_completion_port_add_existing_socket_expectations(test_socket);

    //act
    int result = completion_port_add(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx_2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(1, EPOLL_CTL_MOD, test_socket, TEST_INBOUND_AND_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_008: [ If the socket entry is registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_010: [ If epoll_ctl succeeds and epoll_op does not contain EPOLLOUT, completion_port_add shall append the operation to the inbound operation queue of the socket entry. ]
TEST_FUNCTION(when_inbound_operation_is_added_while_outbound_operation_is_pending_then_completion_port_add_keeps_both_directions_registered)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);

    setup_completion_port_add_existing_socket_expectations(test_socket);

    //act
    int result = completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx_2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(1, EPOLL_CTL_MOD, test_socket, TEST_INBOUND_AND_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_008: [ If the socket entry is registered in epoll, completion_port_add shall call epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry including the new operation and the socket entry as data.ptr. ]
TEST_FUNCTION(when_second_outbound_operation_is_added_then_completion_port_add_keeps_the_outbound_interest)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);

    setup_completion_port_add_existing_socket_expectations(test_socket);

    //act
    int result = completion_port_add(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx_2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(1, EPOLL_CTL_MOD, test_socket, TEST_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_006: [ If no socket entry exists for socket, completion_port_add shall allocate a new socket entry and add it to the socket entry lookup. ]
TEST_FUNCTION(when_operations_are_added_for_sockets_in_the_same_bucket_then_each_socket_gets_its_own_socket_entry)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    setup_completion_port_add_new_socket_expectations(test_socket_same_bucket);
    setup_completion_port_add_existing_socket_expectations(test_socket);

    //act
    int result_1 = completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket_same_bucket, test_port_event_complete, test_callback_ctx_2);
    int result_2 = completion_port_add(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx_3);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 3, g_captured_epoll_ctl_call_count);
    ASSERT_ARE_NOT_EQUAL(void_ptr, socket_entry, g_captured_epoll_ctl_calls[1].data_ptr);
    assert_captured_epoll_ctl_call(1, EPOLL_CTL_ADD, test_socket_same_bucket, TEST_INBOUND_INTEREST, g_captured_epoll_ctl_calls[1].data_ptr);
    assert_captured_epoll_ctl_call(2, EPOLL_CTL_MOD, test_socket, TEST_INBOUND_AND_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
TEST_FUNCTION(completion_port_add_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_completion_port_add_new_socket_expectations(test_socket);

    umock_c_negative_tests_snapshot();

    //act
    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            errno = EACCES;
            int result = completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", index);
        }
    }

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_011: [ If epoll_ctl with EPOLL_CTL_ADD fails, completion_port_add shall remove the new socket entry from the socket entry lookup and free it. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_013: [ If queueing the operation fails, completion_port_add shall free the pending operation. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
TEST_FUNCTION(when_epoll_ctl_ADD_fails_then_completion_port_add_frees_the_socket_entry_and_the_operation)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_completion_port_add_begin_expectations();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_ADD, test_socket, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // socket entry
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // operation
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    //act
    int result = completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_captured_epoll_ctl_call(0, EPOLL_CTL_ADD, test_socket, TEST_INBOUND_INTEREST, g_captured_epoll_ctl_calls[0].data_ptr);

    // the socket entry is gone, so the next operation registers the socket again
    umock_c_reset_all_calls();
    setup_completion_port_add_new_socket_expectations(test_socket);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_013: [ If queueing the operation fails, completion_port_add shall free the pending operation. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
TEST_FUNCTION(when_epoll_ctl_MOD_fails_then_completion_port_add_does_not_queue_the_operation)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    setup_completion_port_add_begin_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG))
        .SetReturn(-1);
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // operation
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    //act
    int result = completion_port_add(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx_2);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // only the operation that was queued is abandoned
    umock_c_reset_all_calls();
    setup_completion_port_remove_begin_expectations(test_socket);
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    setup_completion_port_remove_retire_expectations();
    setup_completion_port_remove_end_expectations();
    completion_port_remove(port_handle, test_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_019: [ completion_port_add shall ensure the thread completion flag is not set. ]
TEST_FUNCTION(completion_port_add_dec_ref_running_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// completion_port_remove

// Tests_SRS_COMPLETION_PORT_LINUX_11_028: [ If completion_port is NULL, completion_port_remove shall return. ]
TEST_FUNCTION(completion_port_remove_completion_port_NULL_fail)
{
    //arrange

    //act
    completion_port_remove(NULL, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_029: [ If socket is INVALID_SOCKET, completion_port_remove shall return. ]
TEST_FUNCTION(completion_port_remove_socket_INVALID_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    //act
    completion_port_remove(port_handle, INVALID_SOCKET);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_014: [ completion_port_remove shall acquire the completion port lock. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_030: [ completion_port_remove shall remove the underlying socket from the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_015: [ If no socket entry exists for socket, completion_port_remove shall not call any event_callback. ]
TEST_FUNCTION(when_socket_was_never_added_then_completion_port_remove_only_calls_EPOLL_CTL_DEL)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket_same_bucket, test_callback_ctx);

    setup_completion_port_remove_begin_expectations(test_socket);
    setup_completion_port_remove_end_expectations();

    //act
    completion_port_remove(port_handle, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_016: [ completion_port_remove shall remove the socket entry from the socket entry lookup. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_018: [ completion_port_remove shall release the completion port lock before calling any event_callback. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_019: [ completion_port_remove shall call the event_callback of every pending operation of the socket entry with COMPLETION_PORT_EPOLL_ABANDONED. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_020: [ completion_port_remove shall free every abandoned operation. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_039: [ If the event_callback is currently executing, completion_port_remove shall wait for the event_callback function to finish before completing. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_021: [ completion_port_remove shall add the socket entry to the retired list of the completion port. ]
TEST_FUNCTION(when_operations_are_pending_in_both_directions_then_completion_port_remove_abandons_each_of_them_once)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket_same_bucket, test_callback_ctx_3);

    setup_completion_port_remove_begin_expectations(test_socket);
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    setup_completion_port_remove_retire_expectations();
    setup_completion_port_remove_end_expectations();

    //act
    completion_port_remove(port_handle, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // a second remove finds nothing left to abandon
    umock_c_reset_all_calls();
    setup_completion_port_remove_begin_expectations(test_socket);
    setup_completion_port_remove_end_expectations();
    completion_port_remove(port_handle, test_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // the socket entry left the lookup, so a new operation registers the socket again
    umock_c_reset_all_calls();
    setup_completion_port_add_new_socket_expectations(test_socket);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_port_event_complete, test_callback_ctx));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_039: [ If the event_callback is currently executing, completion_port_remove shall wait for the event_callback function to finish before completing. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_019: [ completion_port_remove shall call the event_callback of every pending operation of the socket entry with COMPLETION_PORT_EPOLL_ABANDONED. ]
TEST_FUNCTION(when_completion_port_remove_runs_while_the_worker_executes_a_callback_then_only_the_pending_operation_is_abandoned)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);
    g_remove_during_callback_completion_port = port_handle;
    g_remove_during_callback_socket = test_socket;

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    // completion_port_remove from another thread, while the inbound callback executes
    setup_completion_port_remove_begin_expectations(test_socket);
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    // the inbound callback is still executing, a real wait would block until the worker finishes it
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_OK);
    setup_lock_expectations();
    setup_unlock_expectations();
    setup_completion_port_remove_end_expectations();
    // back on the worker
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// epoll worker func

// Tests_SRS_COMPLETION_PORT_LINUX_11_031: [ If parameter is NULL, epoll_worker_func shall do nothing. ]
TEST_FUNCTION(epoll_worker_func_parameter_NULL_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    //act
    g_saved_worker_thread_func(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_022: [ Before each call to epoll_wait, epoll_worker_func shall free the socket entries in the retired list. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_032: [ epoll_worker_func shall call epoll_wait to wait for an epoll event to become signaled with a timeout of 2 Seconds. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_timeout_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    (void)add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    setup_free_retired_socket_entries_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .SetReturn(0);
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_033: [ On a epoll_wait timeout epoll_worker_func shall ensure it should not exit and issue another epoll_wait. ]
TEST_FUNCTION(when_epoll_wait_times_out_then_epoll_worker_func_calls_epoll_wait_again)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();

    setup_free_retired_socket_entries_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    setup_free_retired_socket_entries_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .SetReturn(0);
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_023: [ For each event, epoll_worker_func shall acquire the completion port lock. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_027: [ If an operation was removed from a queue, epoll_worker_func shall increment the executing callback count of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_029: [ If no operation is pending for the socket entry, epoll_worker_func shall not re-arm the socket. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_030: [ epoll_worker_func shall release the completion port lock before calling any event_callback. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_032: [ Otherwise, epoll_worker_func shall call the event_callback of the removed inbound operation with COMPLETION_PORT_EPOLL_EPOLLIN. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_034: [ epoll_worker_func shall free the removed operations after calling their event_callback. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_035: [ After calling the event callbacks, epoll_worker_func shall decrement the executing callback count of the socket entry by calling InterlockedHL_DecrementAndWake. ]
TEST_FUNCTION(when_event_contains_EPOLLIN_then_epoll_worker_func_completes_the_inbound_operation_with_EPOLLIN)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_031: [ If the removed inbound operation was added with EPOLLRDHUP in epoll_op and the event contains EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall call its event_callback with COMPLETION_PORT_EPOLL_EPOLLRDHUP. ]
TEST_FUNCTION(when_event_contains_EPOLLRDHUP_then_epoll_worker_func_completes_the_inbound_operation_with_EPOLLRDHUP)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN | EPOLLRDHUP, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_031: [ If the removed inbound operation was added with EPOLLRDHUP in epoll_op and the event contains EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall call its event_callback with COMPLETION_PORT_EPOLL_EPOLLRDHUP. ]
TEST_FUNCTION(when_event_contains_only_EPOLLHUP_then_epoll_worker_func_completes_the_inbound_operation_with_EPOLLRDHUP)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLHUP, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_032: [ Otherwise, epoll_worker_func shall call the event_callback of the removed inbound operation with COMPLETION_PORT_EPOLL_EPOLLIN. ]
TEST_FUNCTION(when_inbound_operation_did_not_ask_for_EPOLLRDHUP_then_epoll_worker_func_completes_it_with_EPOLLIN_on_EPOLLRDHUP)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_NOTIFY_IN_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN | EPOLLRDHUP, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_026: [ If the event contains EPOLLOUT, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the outbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_033: [ epoll_worker_func shall call the event_callback of the removed outbound operation with COMPLETION_PORT_EPOLL_EPOLLOUT. ]
TEST_FUNCTION(when_event_contains_EPOLLOUT_then_epoll_worker_func_completes_the_outbound_operation_with_EPOLLOUT)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLOUT, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_026: [ If the event contains EPOLLOUT, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the outbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_029: [ If no operation is pending for the socket entry, epoll_worker_func shall not re-arm the socket. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_034: [ epoll_worker_func shall free the removed operations after calling their event_callback. ]
TEST_FUNCTION(when_event_contains_EPOLLIN_and_EPOLLOUT_then_epoll_worker_func_completes_one_operation_per_direction)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN | EPOLLOUT, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_026: [ If the event contains EPOLLOUT, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the outbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_031: [ If the removed inbound operation was added with EPOLLRDHUP in epoll_op and the event contains EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall call its event_callback with COMPLETION_PORT_EPOLL_EPOLLRDHUP. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_033: [ epoll_worker_func shall call the event_callback of the removed outbound operation with COMPLETION_PORT_EPOLL_EPOLLOUT. ]
TEST_FUNCTION(when_event_contains_EPOLLERR_then_epoll_worker_func_completes_one_operation_per_direction)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLERR, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_028: [ If any operation is still pending for the socket entry, epoll_worker_func shall re-arm the socket by calling epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry and the socket entry as data.ptr. ]
TEST_FUNCTION(when_an_outbound_operation_remains_pending_then_epoll_worker_func_re_arms_the_socket_for_EPOLLOUT)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 3, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(2, EPOLL_CTL_MOD, test_socket, TEST_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_026: [ If the event contains EPOLLOUT, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the outbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_028: [ If any operation is still pending for the socket entry, epoll_worker_func shall re-arm the socket by calling epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry and the socket entry as data.ptr. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_029: [ If no operation is pending for the socket entry, epoll_worker_func shall not re-arm the socket. ]
TEST_FUNCTION(when_two_outbound_operations_are_pending_then_epoll_worker_func_completes_them_in_order_one_per_event)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);
    (void)add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx_2);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLOUT, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 3, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(2, EPOLL_CTL_MOD, test_socket, TEST_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_025: [ If the event contains EPOLLIN, EPOLLRDHUP, EPOLLERR or EPOLLHUP, epoll_worker_func shall remove the first operation, if any, from the inbound operation queue of the socket entry. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_028: [ If any operation is still pending for the socket entry, epoll_worker_func shall re-arm the socket by calling epoll_ctl with EPOLL_CTL_MOD, the epoll interest of the socket entry and the socket entry as data.ptr. ]
TEST_FUNCTION(when_event_matches_no_pending_operation_then_epoll_worker_func_only_re_arms_the_socket)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG));
    setup_unlock_expectations();
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, g_captured_epoll_ctl_call_count);
    assert_captured_epoll_ctl_call(1, EPOLL_CTL_MOD, test_socket, TEST_OUTBOUND_INTEREST, socket_entry);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_034: [ epoll_worker_func shall loop through the num of descriptors that was returned. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_multiple_values_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_SEND_EPOLL_OP, test_socket, test_callback_ctx);
    void* socket_entry_2 = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket_2, test_callback_ctx_2);

    struct epoll_event events[2];
    set_event(&events[0], EPOLLOUT, socket_entry);
    set_event(&events[1], EPOLLIN | EPOLLRDHUP, socket_entry_2);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 2);
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_lock_expectations();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx_2, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_DecrementAndWake(IGNORED_ARG));
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_01_017: [ completion_port_remove shall mark the socket entry as removed. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_024: [ If the socket entry of the event is marked as removed, epoll_worker_func shall skip the event. ]
// Tests_SRS_COMPLETION_PORT_LINUX_01_022: [ Before each call to epoll_wait, epoll_worker_func shall free the socket entries in the retired list. ]
TEST_FUNCTION(when_the_socket_is_removed_after_epoll_wait_returned_then_epoll_worker_func_skips_the_event_and_frees_the_socket_entry_before_the_next_epoll_wait)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = create_completion_port();
    void* socket_entry = add_operation(port_handle, TEST_RECEIVE_EPOLL_OP, test_socket, test_callback_ctx);
    g_remove_during_epoll_wait_completion_port = port_handle;
    g_remove_during_epoll_wait_socket = test_socket;

    struct epoll_event events[1];
    set_event(&events[0], EPOLLIN, socket_entry);

    setup_free_retired_socket_entries_expectations();
    setup_epoll_wait_expectations(events, 1);
    // completion_port_remove from another thread, after epoll_wait returned the event
    setup_completion_port_remove_begin_expectations(test_socket);
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    setup_completion_port_remove_retire_expectations();
    setup_completion_port_remove_end_expectations();
    // back on the worker, the event of the removed socket is skipped
    setup_lock_expectations();
    setup_unlock_expectations();
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    // the retired socket entry is freed before the next epoll_wait
    setup_free_retired_socket_entries_expectations();
    STRICT_EXPECTED_CALL(free(socket_entry));
    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .SetReturn(0);
    setup_worker_exit_expectations();

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
