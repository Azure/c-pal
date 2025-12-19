//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "testrunnerswitcher.h"

#include "c_pal/sync.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"

#include "file_int_helpers.h"

#include "c_pal/file.h"

TEST_DEFINE_ENUM_TYPE(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_RESULT)
TEST_DEFINE_ENUM_TYPE(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_RESULT)

typedef struct WRITE_COMPLETE_CONTEXT_TAG
{
    int32_t pre_callback_value;
    volatile_atomic int32_t value;
    int32_t post_callback_value;
    bool did_write_succeed;
}WRITE_COMPLETE_CONTEXT;

typedef struct READ_COMPLETE_CONTEXT_TAG
{
    int32_t pre_callback_value;
    volatile_atomic int32_t value;
    int32_t post_callback_value;
    bool did_read_succeed;
}READ_COMPLETE_CONTEXT;

static void write_callback(void* context, bool is_successful)
{
    WRITE_COMPLETE_CONTEXT* write_context = (WRITE_COMPLETE_CONTEXT*)context;
    ASSERT_ARE_EQUAL(int32_t, write_context->pre_callback_value, interlocked_add(&write_context->value, 0));
    write_context->did_write_succeed = is_successful;
    (void)interlocked_exchange(&write_context->value, write_context->post_callback_value);
    wake_by_address_single(&write_context->value);
}

static void read_callback(void* context, bool is_successful)
{
    READ_COMPLETE_CONTEXT* read_context = (READ_COMPLETE_CONTEXT*)context;
    ASSERT_ARE_EQUAL(int32_t, read_context->pre_callback_value, interlocked_add(&read_context->value, 0));
    read_context->did_read_succeed = is_successful;
    (void)interlocked_exchange(&read_context->value, read_context->post_callback_value);
    wake_by_address_single(&read_context->value);
}

static void wait_on_address_helper(volatile_atomic int32_t* address, int32_t old_value, uint32_t timeout)
{
    int32_t current_value;
    do
    {
        wait_on_address(address, old_value, timeout);
        current_value = interlocked_add(address, 0);
    } while (current_value == old_value);
}

static FILE_HANDLE file_create_helper(const char* filename)
{
    (void)delete_file(filename);
    ASSERT_IS_FALSE(check_file_exists(filename));

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    FILE_HANDLE file_handle = file_create(execution_engine, filename, NULL, NULL);
    ASSERT_IS_NOT_NULL(file_handle);

    execution_engine_dec_ref(execution_engine);

    return file_handle;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(b)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
}

TEST_FUNCTION_CLEANUP(d)
{
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
TEST_FUNCTION(file_create_creates_new_file)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    char filename[] = "file_create_creates_new_file.txt";
    (void)delete_file(filename);
    ASSERT_IS_FALSE(check_file_exists(filename));

    ///act
    FILE_HANDLE file_handle = file_create(execution_engine, filename, NULL, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(file_handle);
    ASSERT_IS_TRUE(check_file_exists(filename));

    ///cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: [If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_to_a_file_and_read_from_it)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    char filename[] = "write_to_a_file_and_read_from_it.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ///act
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source, size, 0, write_callback, &write_context));
    
    ///assert
    wait_on_address_helper(&write_context.value, write_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ///act
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, file_read_async(file_handle, destination, sizeof(destination), 0, read_callback, &read_context));

    ///assert
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);
    ASSERT_ARE_EQUAL(char_ptr, source, destination);

    //cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_twice_to_a_file_contiguously_and_read_from_it)
{
    ///arrange

    unsigned char source1[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context1;
    write_context1.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context1.value, write_context1.pre_callback_value);
    write_context1.post_callback_value = 42;

    unsigned char source2[] = "efgh";
    WRITE_COMPLETE_CONTEXT write_context2;
    write_context2.pre_callback_value = 45;
    (void)interlocked_exchange(&write_context2.value, write_context2.pre_callback_value);
    write_context2.post_callback_value = 46;

    unsigned char destination[9];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    char filename[] = "write_twice_to_a_file_contiguously_and_read_from_it.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ///act
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source1, sizeof(source1) - 1 , 0, write_callback, &write_context1));
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source2, sizeof(source2), sizeof(source1) - 1, write_callback, &write_context2));
   
    ///assert
    wait_on_address_helper(&write_context1.value, write_context1.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context1.post_callback_value, interlocked_or(&write_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context1.did_write_succeed);

    wait_on_address_helper(&write_context2.value, write_context2.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context2.post_callback_value, interlocked_or(&write_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context2.did_write_succeed);

    ///act
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, file_read_async(file_handle, destination, sizeof(destination), 0, read_callback, &read_context));

    ///assert
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);
    ASSERT_ARE_EQUAL(char_ptr, "abcdefgh", destination);

    //cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: [If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(write_twice_to_a_file_non_contiguously_and_read_from_it)
{
    ///arrange
    const int size = 5;

    unsigned char source1[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context1;
    write_context1.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context1.value, write_context1.pre_callback_value);
    write_context1.post_callback_value = 42;


    unsigned char source2[] = "efgh";
    WRITE_COMPLETE_CONTEXT write_context2;
    write_context2.pre_callback_value = 45;
    (void)interlocked_exchange(&write_context2.value, write_context2.pre_callback_value);
    write_context2.post_callback_value = 46;


    unsigned char destination1[5];
    READ_COMPLETE_CONTEXT read_context1;
    read_context1.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context1.value, read_context1.pre_callback_value);
    read_context1.post_callback_value = 44;


    unsigned char destination2[5];
    READ_COMPLETE_CONTEXT read_context2;
    read_context2.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context2.value, read_context2.pre_callback_value);
    read_context2.post_callback_value = 44;

    char filename[] = "write_twice_to_a_file_non_contiguously_and_read_from_it.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    uint64_t second_write_position = 50;

    ///act
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source1, size, 0, write_callback, &write_context1));
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source2, size, second_write_position, write_callback, &write_context2));
   
    ///assert
    wait_on_address_helper(&write_context1.value, write_context1.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context1.post_callback_value, interlocked_or(&write_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context1.did_write_succeed);

    wait_on_address_helper(&write_context2.value, write_context2.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context2.post_callback_value, interlocked_or(&write_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context2.did_write_succeed);

    ///act
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, file_read_async(file_handle, destination1, sizeof(destination1), 0, read_callback, &read_context1));
    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, file_read_async(file_handle, destination2, sizeof(destination2), second_write_position, read_callback, &read_context2));

    ///assert
    wait_on_address_helper(&read_context1.value, read_context1.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context1.post_callback_value, interlocked_or(&read_context1.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context1.did_read_succeed);

    wait_on_address_helper(&read_context2.value, read_context2.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context2.post_callback_value, interlocked_or(&read_context2.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context2.did_read_succeed);

    ASSERT_ARE_EQUAL(char_ptr, source1, destination1);
    ASSERT_ARE_EQUAL(char_ptr, source2, destination2);

    //cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: [If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(perform_operations_open_write_close_open_read_close)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;


    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    char filename[] = "perform_operations_open_write_close_open_read_close.txt";
    (void)delete_file(filename);
    ASSERT_IS_FALSE(check_file_exists(filename));
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    ///act
    FILE_HANDLE file_handle1 = file_create(execution_engine, filename, NULL, NULL);
    ASSERT_IS_NOT_NULL(file_handle1);

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle1, source, size, 0, write_callback, &write_context));
    wait_on_address_helper(&write_context.value, write_context.pre_callback_value, UINT32_MAX);
    file_destroy(file_handle1);
   
    ///assert
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ///act
    FILE_HANDLE file_handle2 = file_create(execution_engine, filename, NULL, NULL);
    ASSERT_IS_NOT_NULL(file_handle2);

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_OK, file_read_async(file_handle2, destination, sizeof(destination), 0, read_callback, &read_context));
    file_destroy(file_handle2);

    ///assert
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);
    ASSERT_ARE_EQUAL(char_ptr, source, destination);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
TEST_FUNCTION(read_across_eof_fails)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    uint32_t read_position = 2;

    char filename[] = "read_across_eof_fails.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ///act
    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source, size, 0, write_callback, &write_context));

    ///assert

    wait_on_address_helper(&write_context.value, write_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ///act
    file_read_async(file_handle, destination, sizeof(destination), read_position, read_callback, &read_context);

    ///assert
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_FALSE(read_context.did_read_succeed);

    //cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
}

/*Tests_SRS_FILE_43_039: [ If position + size exceeds the size of the file, user_callback shall be called with success as false. ]*/
TEST_FUNCTION(read_beyond_eof_fails)
{
    ///arrange
    const int size = 5;

    unsigned char source[] = "abcd";
    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    unsigned char destination[5];
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 43;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 44;

    uint32_t read_position = 5;

    char filename[] = "read_beyond_eof_fails.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source, size, 0, write_callback, &write_context));
    

    wait_on_address_helper(&write_context.value, write_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    ///act
    file_read_async(file_handle, destination, sizeof(destination), read_position, read_callback, &read_context);

    ///assert
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_FALSE(read_context.did_read_succeed);

    //cleanup
    file_destroy(file_handle);
    (void)delete_file(filename);
}

/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(large_simultaneous_writes_succeed)
{
    ///arrange
    int block_size = 4096;
    int num_blocks = 50;
    WRITE_COMPLETE_CONTEXT contexts[50];
    unsigned char* sources[50];

    char filename[] = "large_simultaneous_writes_succeed.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ///act
    for (int i = 0; i < num_blocks; ++i)
    {
        sources[i] = malloc(block_size);
        ASSERT_IS_NOT_NULL(sources[i]);
        (void)memset(sources[i], 'a' + i, block_size);
        contexts[i].pre_callback_value = num_blocks + 1;
        (void)interlocked_exchange(&contexts[i].value, contexts[i].pre_callback_value);
        contexts[i].post_callback_value = i;

        ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, sources[i], block_size, block_size * i, write_callback, &contexts[i]));
    
    }

    for (int i = 0; i < num_blocks; ++i)
    {
        wait_on_address_helper(&contexts[i].value, contexts[i].pre_callback_value, UINT32_MAX);
        ASSERT_ARE_EQUAL(int32_t, contexts[i].post_callback_value, interlocked_or(&contexts[i].value, 0), "value should be post_callback_value");
        ASSERT_IS_TRUE(contexts[i].did_write_succeed);
    }

    ///assert
    unsigned char* destination = malloc_2(block_size, num_blocks);
    ASSERT_IS_NOT_NULL(destination);
    READ_COMPLETE_CONTEXT read_context;
    read_context.pre_callback_value = 0;
    (void)interlocked_exchange(&read_context.value, read_context.pre_callback_value);
    read_context.post_callback_value = 1;

    ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_read_async(file_handle, destination, block_size * num_blocks, 0, read_callback, &read_context));
    wait_on_address_helper(&read_context.value, read_context.pre_callback_value, UINT32_MAX);
    ASSERT_ARE_EQUAL(int32_t, read_context.post_callback_value, interlocked_or(&read_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(read_context.did_read_succeed);

    for (int i = 0; i < num_blocks; ++i)
    {
        ASSERT_ARE_EQUAL(int, 0, memcmp(&destination[i* block_size], sources[i], block_size));
    }


    //cleanup
    free(destination);
    for (int i = 0; i < num_blocks; ++i)
    {
        free(sources[i]);
    }
    file_destroy(file_handle);
    (void)delete_file(filename);
} 


/*Tests_SRS_FILE_43_003: [If a file with name full_file_name does not exist, file_create shall create a file with that name.]*/
/*Tests_SRS_FILE_43_001: [file_create shall open the file named full_file_name for asynchronous operations and return its handle.]*/
/*Tests_SRS_FILE_43_006: [file_destroy shall wait for all pending I/O operations to complete.]*/
/*Tests_SRS_FILE_43_007: [file_destroy shall close the file handle handle.]*/
/*Tests_SRS_FILE_43_014: [file_write_async shall enqueue a write request to write source's content to the position offset in the file. ]*/
/*Tests_SRS_FILE_43_041: If position + size is greater than the size of the file and the call to write is successfull, file_write_async shall grow the file to accomodate the write.]*/
/*Tests_SRS_FILE_43_008: [file_write_async shall call user_call_back passing user_context and success depending on the success of the asynchronous write operation.]*/
/*Tests_SRS_FILE_43_030: [file_write_async shall succeed and return FILE_WRITE_ASYNC_OK.]*/
/*Tests_SRS_FILE_43_021: [file_read_async shall enqueue a read request to read handle's content at position offset and write it to destination. ]*/
/*Tests_SRS_FILE_43_016: [file_read_async shall call user_callback passing user_context and success depending on the success of the asynchronous read operation.]*/
/*Tests_SRS_FILE_43_031: [file_read_async shall succeed and return FILE_READ_ASYNC_OK.]*/
TEST_FUNCTION(large_simultaneous_reads_succeed)
{
    ///arrange
    int block_size = 4096;
    int num_blocks = 50;
    unsigned char* source = malloc_2(block_size, num_blocks);
    ASSERT_IS_NOT_NULL(source);

    for (int i = 0; i < num_blocks; ++i)
    {
        (void)memset(&source[i * block_size], 'a' + i, block_size);
    }

    WRITE_COMPLETE_CONTEXT write_context;
    write_context.pre_callback_value = 41;
    (void)interlocked_exchange(&write_context.value, write_context.pre_callback_value);
    write_context.post_callback_value = 42;

    char filename[] = "large_simultaneous_reads_succeed.txt";
    FILE_HANDLE file_handle = file_create_helper(filename);

    ASSERT_ARE_EQUAL(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_write_async(file_handle, source, block_size * num_blocks, 0, write_callback, &write_context));
    wait_on_address_helper(&write_context.value, write_context.pre_callback_value, UINT32_MAX);

    ASSERT_ARE_EQUAL(int32_t, write_context.post_callback_value, interlocked_or(&write_context.value, 0), "value should be post_callback_value");
    ASSERT_IS_TRUE(write_context.did_write_succeed);

    READ_COMPLETE_CONTEXT contexts[50];
    unsigned char* destinations[50];

    ///act
    for (int i = 0; i < num_blocks; ++i)
    {
        destinations[i] = malloc(block_size);
        ASSERT_IS_NOT_NULL(destinations[i]);
        contexts[i].pre_callback_value = num_blocks + 1;
        (void)interlocked_exchange(&contexts[i].value, contexts[i].pre_callback_value);
        contexts[i].post_callback_value = i;

        ASSERT_ARE_EQUAL(FILE_READ_ASYNC_RESULT, FILE_WRITE_ASYNC_OK, file_read_async(file_handle, destinations[i], block_size, block_size * i, read_callback, &contexts[i]));
    }

    for (int i = 0; i < num_blocks; ++i)
    {
        wait_on_address_helper(&contexts[i].value, contexts[i].pre_callback_value, UINT32_MAX);
        ASSERT_ARE_EQUAL(int32_t, contexts[i].post_callback_value, interlocked_or(&contexts[i].value, 0), "value should be post_callback_value");
        ASSERT_IS_TRUE(contexts[i].did_read_succeed);
    }

    ///assert
    for (int i = 0; i < num_blocks; ++i)
    {
        ASSERT_ARE_EQUAL(int, 0, memcmp(&source[i * block_size], destinations[i], block_size));
    }

    //cleanup
    free(source);
    for (int i = 0; i < num_blocks; ++i)
    {
        free(destinations[i]);
    }
    file_destroy(file_handle);
    (void)delete_file(filename);
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
