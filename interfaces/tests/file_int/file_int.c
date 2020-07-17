//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#endif


#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "azure_c_pal/sync.h"
#include "azure_c_pal/file.h"
#include "azure_c_pal/interlocked.h"


TEST_DEFINE_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT)
TEST_DEFINE_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT)

typedef struct WRITE_COMPLETE_CONTEXT_TAG
{
    int32_t pre_callback_value;
    volatile int32_t value;
    int32_t post_callback_value;
    bool did_write_succeed;
}WRITE_COMPLETE_CONTEXT;

typedef struct READ_COMPLETE_CONTEXT_TAG
{
    int32_t pre_callback_value;
    volatile int32_t value;
    int32_t post_callback_value;
    bool did_read_succeed;
}READ_COMPLETE_CONTEXT;

static void write_callback(void* context, bool is_successful) {
    WRITE_COMPLETE_CONTEXT* write_context = (WRITE_COMPLETE_CONTEXT*)context;
    ASSERT_ARE_EQUAL(int32_t, write_context->pre_callback_value, write_context->value);
    interlocked_exchange(&(write_context->value), write_context->post_callback_value);
    write_context->did_write_succeed = is_successful;
    wake_by_address_single(&write_context->value);
}

static void read_callback(void* context, bool is_successful) {
    READ_COMPLETE_CONTEXT* read_context = (READ_COMPLETE_CONTEXT*)context;
    ASSERT_ARE_EQUAL(int32_t, read_context->pre_callback_value, read_context->value);
    interlocked_exchange(&(read_context->value), read_context->post_callback_value);
    read_context->did_read_succeed = is_successful;
    wake_by_address_single(&(read_context->value));
}


BEGIN_TEST_SUITE(file_int)


TEST_SUITE_INITIALIZE(a)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(b)
{

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(c)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001 : [file_create shall open the file named full_file_name for asynchronous operationsand return its handle.]*/
/*Tests_SRS_FILE_43_006 : [file_destroy shall wait for all pending I / O operations to complete.]*/
/*Tests_SRS_FILE_43_007 : [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014 : [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041:[If position + size is greater than the size of the file and the call to write is succesfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008 : [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030 : [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021 : [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016:[file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031 : [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_to_a_file_and_read_from_it)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;



    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    FILE_HANDLE file_handle = file_create(execution_engine, "write_to_a_file_and_read_from_it.txt", NULL, NULL);

    ///act
    FILE_WRITE_ASYNC_RESULT write_result = file_write_async(file_handle, source, size, 0, write_callback, &write_context);
    wait_on_address(&write_context.value, &write_context.pre_callback_value, UINT32_MAX);
    FILE_READ_ASYNC_RESULT read_result = file_read_async(file_handle, destination, size, 0, read_callback, &read_context);
    wait_on_address(&read_context.value, &read_context.pre_callback_value, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, read_result);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);
    ASSERT_ARE_EQUAL(char_ptr, source, destination);

    //cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001 : [file_create shall open the file named full_file_name for asynchronous operationsand return its handle.]*/
/*Tests_SRS_FILE_43_006 : [file_destroy shall wait for all pending I / O operations to complete.]*/
/*Tests_SRS_FILE_43_007 : [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014 : [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041:[If position + size is greater than the size of the file and the call to write is succesfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008 : [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030 : [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021 : [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016:[file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031 : [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_twice_to_a_file_contiguously_and_read_from_it)
{
    ///arrange
    const int size = 5;

    unsigned char source1[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context1;
    write_context1.pre_callback_value = 41;
    interlocked_exchange(&write_context1.value, write_context1.pre_callback_value);
    write_context1.post_callback_value = 42;

    unsigned char source2[] = "efgh";
    WRITE_COMPLETE_CONTEXT write_context2;
    write_context2.pre_callback_value = 45;
    interlocked_exchange(&write_context2.value, write_context2.pre_callback_value);
    write_context2.post_callback_value = 46;

    unsigned char destination[9];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;


    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    FILE_HANDLE file_handle = file_create(execution_engine, "write_twice_to_a_file_contiguously_and_read_from_it.txt", NULL, NULL);

    ///act
    FILE_WRITE_ASYNC_RESULT write_result1 = file_write_async(file_handle, source1, size, 0, write_callback, &write_context1);
    wait_on_address(&write_context1.value, &write_context1.pre_callback_value, UINT32_MAX);
    FILE_WRITE_ASYNC_RESULT write_result2 = file_write_async(file_handle, source2, size, 4, write_callback, &write_context2);
    wait_on_address(&write_context2.value, &write_context2.pre_callback_value, UINT32_MAX);
    FILE_READ_ASYNC_RESULT read_result = file_read_async(file_handle, destination, 9, 0, read_callback, &read_context);
    wait_on_address(&read_context.value, &read_context.pre_callback_value, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result1);
    ASSERT_ARE_EQUAL(int32_t, write_context1.post_callback_value, interlocked_or(&write_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context1.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result2);
    ASSERT_ARE_EQUAL(int32_t, write_context2.post_callback_value, interlocked_or(&write_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context2.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, read_result);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);

    ASSERT_ARE_EQUAL(char_ptr, "abcdefgh", destination);

    //cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001 : [file_create shall open the file named full_file_name for asynchronous operationsand return its handle.]*/
/*Tests_SRS_FILE_43_006 : [file_destroy shall wait for all pending I / O operations to complete.]*/
/*Tests_SRS_FILE_43_007 : [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014 : [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041:[If position + size is greater than the size of the file and the call to write is succesfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008 : [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030 : [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021 : [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016:[file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031 : [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_twice_to_a_file_non_contiguously_and_read_from_it)
{
    ///arrange
    const int size = 5;

    unsigned char source1[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context1;
    write_context1.pre_callback_value = 41;
    interlocked_exchange(&write_context1.value, write_context1.pre_callback_value);
    write_context1.post_callback_value = 42;


    unsigned char source2[] = "efgh";
    WRITE_COMPLETE_CONTEXT write_context2;
    write_context2.pre_callback_value = 45;
    interlocked_exchange(&write_context2.value, write_context2.pre_callback_value);
    write_context2.post_callback_value = 46;


    unsigned char destination1[5];
    READ_COMPLETE_CONTEXT read_context1;
    read_context1.pre_callback_value = 43;
    interlocked_exchange(&read_context1.value, read_context1.pre_callback_value);
    read_context1.post_callback_value = 44;


    unsigned char destination2[5];
    READ_COMPLETE_CONTEXT read_context2;
    read_context2.pre_callback_value = 43;
    interlocked_exchange(&read_context2.value, read_context2.pre_callback_value);
    read_context2.post_callback_value = 44;


    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    FILE_HANDLE file_handle = file_create(execution_engine, "write_twice_to_a_file_non_contiguously_and_read_from_it.txt", NULL, NULL);

    uint64_t second_write_position = 50;

    ///act
    FILE_WRITE_ASYNC_RESULT write_result1 = file_write_async(file_handle, source1, size, 0, write_callback, &write_context1);
    wait_on_address(&write_context1.value, &write_context1.pre_callback_value, UINT32_MAX);
    FILE_WRITE_ASYNC_RESULT write_result2 = file_write_async(file_handle, source2, size, second_write_position, write_callback, &write_context2);
    wait_on_address(&write_context2.value, &write_context2.pre_callback_value, UINT32_MAX);
    FILE_READ_ASYNC_RESULT read_result1 = file_read_async(file_handle, destination1, size, 0, read_callback, &read_context1);
    wait_on_address(&read_context1.value, &read_context1.pre_callback_value, UINT32_MAX);
    FILE_READ_ASYNC_RESULT read_result2 = file_read_async(file_handle, destination2, size, second_write_position, read_callback, &read_context2);
    wait_on_address(&read_context2.value, &read_context2.pre_callback_value, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result1);
    ASSERT_ARE_EQUAL(int32_t, write_context1.post_callback_value, interlocked_or(&write_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context1.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result2);
    ASSERT_ARE_EQUAL(int32_t, write_context2.post_callback_value, interlocked_or(&write_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context2.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, read_result1);
    ASSERT_ARE_EQUAL(int32_t, read_context1.post_callback_value, interlocked_or(&read_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context1.did_read_succeed);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, read_result2);
    ASSERT_ARE_EQUAL(int32_t, read_context2.post_callback_value, interlocked_or(&read_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context2.did_read_succeed);

    ASSERT_ARE_EQUAL(char_ptr, source1, destination1);
    ASSERT_ARE_EQUAL(char_ptr, source2, destination2);

    //cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001 : [file_create shall open the file named full_file_name for asynchronous operationsand return its handle.]*/
/*Tests_SRS_FILE_43_006 : [file_destroy shall wait for all pending I / O operations to complete.]*/
/*Tests_SRS_FILE_43_007 : [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014 : [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041:[If position + size is greater than the size of the file and the call to write is succesfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008 : [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030 : [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021 : [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016:[file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031 : [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(perform_operations_open_write_close_open_read_close)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;


    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;


    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);

    ///act
    FILE_HANDLE file_handle1 = file_create(execution_engine, "perform_operations_open_write_close_open_read_close.txt", NULL, NULL);
    FILE_WRITE_ASYNC_RESULT write_result = file_write_async(file_handle1, source, size, 0, write_callback, &write_context);
    wait_on_address(&write_context.value, &write_context.pre_callback_value, UINT32_MAX);
    file_destroy(file_handle1);
    FILE_HANDLE file_handle2 = file_create(execution_engine, "perform_operations_open_write_close_open_read_close.txt", NULL, NULL);
    FILE_READ_ASYNC_RESULT read_result = file_read_async(file_handle2, destination, size, 0, read_callback, &read_context);
    wait_on_address(&read_context.value, &read_context.pre_callback_value, UINT32_MAX);
    file_destroy(file_handle2);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, read_result);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);
    ASSERT_ARE_EQUAL(char_ptr, source, destination);
}

/*Tests_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
TEST_FUNCTION(read_across_eof_fails)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    uint32_t read_position = 2;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    FILE_HANDLE file_handle = file_create(execution_engine, "read_beyond_eof_fails.txt", NULL, NULL);

    ///act
    FILE_WRITE_ASYNC_RESULT write_result = file_write_async(file_handle, source, size, 0, write_callback, &write_context);
    wait_on_address(&write_context.value, &write_context.pre_callback_value, UINT32_MAX);
    file_read_async(file_handle, destination, size, read_position, read_callback, &read_context);
    wait_on_address(&read_context.value, &read_context.pre_callback_value, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_FALSE(read_context.did_read_succeed);

    //cleanup
    file_destroy(file_handle);
}

/*Tests_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
TEST_FUNCTION(read_beyond_eof_fails)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    uint32_t read_position = 5;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    FILE_HANDLE file_handle = file_create(execution_engine, "read_beyond_eof_fails.txt", NULL, NULL);

    ///act
    FILE_WRITE_ASYNC_RESULT write_result = file_write_async(file_handle, source, size, 0, write_callback, &write_context);
    wait_on_address(&write_context.value, &write_context.pre_callback_value, UINT32_MAX);
    file_read_async(file_handle, destination, size, read_position, read_callback, &read_context);
    wait_on_address(&read_context.value, &read_context.pre_callback_value, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, write_result);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_FALSE(read_context.did_read_succeed);

    //cleanup
    file_destroy(file_handle);
}


END_TEST_SUITE(file_int)
