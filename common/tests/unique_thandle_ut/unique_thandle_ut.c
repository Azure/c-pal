// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "unique_thandle_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, test_object_disposed, int, value);
MOCKABLE_FUNCTION(, void*, type_malloc, size_t, size);
MOCKABLE_FUNCTION(, void*, type_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
MOCKABLE_FUNCTION(, void, type_free, void*, ptr);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

/*a node of a singly linked list, where each node owns the next node*/
typedef struct NODE_TAG NODE;

UNIQUE_THANDLE_TYPE_DECLARE(NODE);

struct NODE_TAG
{
    int value;
    UNIQUE_THANDLE(NODE) next;
};

UNIQUE_THANDLE_TYPE_DEFINE(NODE);

/*a type with a flexible array member*/
typedef struct FLEX_OBJECT_TAG
{
    size_t count;
    int values[];
} FLEX_OBJECT;

UNIQUE_THANDLE_TYPE_DECLARE(FLEX_OBJECT);
UNIQUE_THANDLE_TYPE_DEFINE(FLEX_OBJECT);

/*a type that has its own memory functions*/
typedef struct TYPED_TAG
{
    int value;
} TYPED;

UNIQUE_THANDLE_TYPE_DECLARE(TYPED);
UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(TYPED, type_malloc, type_malloc_flex, type_free);

static void typed_dispose(TYPED* typed)
{
    test_object_disposed(typed->value);
}

/*a type whose UNIQUE_THANDLE functions are named after a name other than the type*/
typedef struct LL_STRUCT_TAG
{
    int value;
} LL_STRUCT;

UNIQUE_THANDLE_MACRO(LL_STRUCT);
UNIQUE_THANDLE_LL_TYPE_DECLARE(LL_NAME, LL_STRUCT);
UNIQUE_THANDLE_LL_TYPE_DEFINE(LL_NAME, LL_STRUCT);

/*a structure that has just been allocated: its field is not initialized*/
typedef struct HOLDER_TAG
{
    UNIQUE_THANDLE(NODE) field;
} HOLDER;

#define UNINITIALIZED_BYTE 0xA5

/*handles with static storage duration, which tests move into and dispose*/
static UNIQUE_THANDLE(NODE) g_file_scope_handle = { { NULL } };
static UNIQUE_THANDLE(NODE) g_file_scope_handle_without_initializer;

static struct G_TAG
{
    UNIQUE_THANDLE(NODE)* observed_handle; /*when not NULL, node_dispose captures the object of *observed_handle in object_at_dispose*/
    const NODE* object_at_dispose;
    UNIQUE_THANDLE(NODE)* reentrant_handle; /*when not NULL, node_dispose disposes *reentrant_handle*/
} g;

static void node_dispose(NODE* node)
{
    if (g.observed_handle != NULL)
    {
        g.object_at_dispose = UNIQUE_THANDLE_GET_T(NODE)(*g.observed_handle);
    }
    else
    {
        /*no handle is observed*/
    }

    if (g.reentrant_handle != NULL)
    {
        UNIQUE_THANDLE_DISPOSE(NODE)(*g.reentrant_handle);
    }
    else
    {
        /*no handle is disposed from here*/
    }

    test_object_disposed(node->value);
    UNIQUE_THANDLE_DISPOSE(NODE)(node->next);
}

/*puts a new node that has no next node in handle, returns 0 on success*/
static int node_create(int value, UNIQUE_THANDLE(NODE) handle)
{
    int result;
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);
    if (node == NULL)
    {
        LogError("failure in UNIQUE_THANDLE_MALLOC(NODE)(node_dispose)");
        result = MU_FAILURE;
    }
    else
    {
        UNIQUE_THANDLE(NODE) no_next = { { NULL } };
        node->value = value;
        UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(node->next, no_next);
        UNIQUE_THANDLE_ADOPT(NODE)(handle, node);
        result = 0;
    }
    return result;
}

/*returns a HOLDER whose bytes are all UNINITIALIZED_BYTE, or NULL on failure*/
static HOLDER* holder_create_uninitialized(void)
{
    HOLDER* result = real_gballoc_hl_malloc(sizeof(HOLDER));
    if (result == NULL)
    {
        LogError("failure in real_gballoc_hl_malloc(sizeof(HOLDER)=%zu)", sizeof(HOLDER));
    }
    else
    {
        (void)memset(result, UNINITIALIZED_BYTE, sizeof(HOLDER));
    }
    return result;
}

static bool holder_is_uninitialized(const HOLDER* holder)
{
    const unsigned char* bytes = (const unsigned char*)holder;
    bool result = true;
    for (size_t i = 0; i < sizeof(HOLDER); i++)
    {
        if (bytes[i] != UNINITIALIZED_BYTE)
        {
            result = false;
            break;
        }
        else
        {
            /*keep checking*/
        }
    }
    return result;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_UMOCK_ALIAS_TYPE(UNIQUE_THANDLE(MOCKED_STRUCT), void*);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(type_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(type_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(type_free, real_gballoc_hl_free);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    (void)memset(&g, 0, sizeof(g));
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* UNIQUE_THANDLE_TYPE_DECLARE */

TEST_FUNCTION(UNIQUE_THANDLE_TYPE_DECLARE_declares_mockable_initialize_move_and_dispose) // no-srs
{
    // arrange
    UNIQUE_THANDLE(MOCKED_STRUCT) t1 = { { NULL } };
    UNIQUE_THANDLE(MOCKED_STRUCT) t2 = { { NULL } };

    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(t1, t2));
    STRICT_EXPECTED_CALL(UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(t1));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(MOCKED_STRUCT)(t1, t2);
    UNIQUE_THANDLE_DISPOSE(MOCKED_STRUCT)(t1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* UNIQUE_THANDLE_TYPE_DEFINE */

/*Tests_SRS_UNIQUE_THANDLE_43_001: [ UNIQUE_THANDLE_TYPE_DEFINE shall use malloc, malloc_flex and free to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_MALLOC_allocates_with_malloc)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);

    // assert
    ASSERT_IS_NOT_NULL(node);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_FREE(NODE)(node);
}

TEST_FUNCTION(UNIQUE_THANDLE_MALLOC_returns_NULL_when_malloc_fails) // no-srs
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    // act
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);

    // assert
    ASSERT_IS_NULL(node);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_001: [ UNIQUE_THANDLE_TYPE_DEFINE shall use malloc, malloc_flex and free to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_MALLOC_FLEX_allocates_with_malloc_flex)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 3, sizeof(int)));

    // act
    FLEX_OBJECT* flex_object = UNIQUE_THANDLE_MALLOC_FLEX(FLEX_OBJECT)(NULL, 3, sizeof(int));

    // assert
    ASSERT_IS_NOT_NULL(flex_object);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    flex_object->count = 3;
    flex_object->values[2] = 42;
    ASSERT_ARE_EQUAL(int, 42, flex_object->values[2]);

    // cleanup
    UNIQUE_THANDLE_FREE(FLEX_OBJECT)(flex_object);
}

/*Tests_SRS_UNIQUE_THANDLE_43_001: [ UNIQUE_THANDLE_TYPE_DEFINE shall use malloc, malloc_flex and free to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_FREE_frees_with_free_without_calling_dispose)
{
    // arrange
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);
    ASSERT_IS_NOT_NULL(node);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_FREE(NODE)(node);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(UNIQUE_THANDLE_GET_T_returns_the_object_of_the_handle) // no-srs
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(7, handle));
    umock_c_reset_all_calls();

    // act
    NODE* node = UNIQUE_THANDLE_GET_T(NODE)(handle);

    // assert
    ASSERT_IS_NOT_NULL(node);
    ASSERT_ARE_EQUAL(int, 7, node->value);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);
}

TEST_FUNCTION(UNIQUE_THANDLE_GET_T_of_a_NULL_handle_returns_NULL) // no-srs
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };

    // act
    NODE* node = UNIQUE_THANDLE_GET_T(NODE)(handle);

    // assert
    ASSERT_IS_NULL(node);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(UNIQUE_THANDLE_GET_T_with_t_NULL_returns_NULL) // no-srs
{
    // arrange

    // act
    NODE* node = UNIQUE_THANDLE_GET_T(NODE)(NULL);

    // assert
    ASSERT_IS_NULL(node);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(UNIQUE_THANDLE_ADOPT_puts_the_object_in_a_handle_that_is_not_initialized) // no-srs
{
    // arrange
    UNIQUE_THANDLE(NODE) no_next = { { NULL } };
    HOLDER* holder = holder_create_uninitialized();
    ASSERT_IS_NOT_NULL(holder);
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);
    ASSERT_IS_NOT_NULL(node);
    node->value = 3;
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(node->next, no_next);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_ADOPT(NODE)(holder->field, node);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, node, UNIQUE_THANDLE_GET_T(NODE)(holder->field));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(holder->field);
    real_gballoc_hl_free(holder);
}

TEST_FUNCTION(UNIQUE_THANDLE_ADOPT_puts_an_object_from_UNIQUE_THANDLE_MALLOC_FLEX_in_a_NULL_handle) // no-srs
{
    // arrange
    UNIQUE_THANDLE(FLEX_OBJECT) flex_handle = { { NULL } };
    FLEX_OBJECT* flex_object = UNIQUE_THANDLE_MALLOC_FLEX(FLEX_OBJECT)(NULL, 1, sizeof(int));
    ASSERT_IS_NOT_NULL(flex_object);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_ADOPT(FLEX_OBJECT)(flex_handle, flex_object);
    FLEX_OBJECT* adopted = UNIQUE_THANDLE_GET_T(FLEX_OBJECT)(flex_handle);
    UNIQUE_THANDLE_DISPOSE(FLEX_OBJECT)(flex_handle);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, flex_object, adopted);
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(FLEX_OBJECT)(flex_handle));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(UNIQUE_THANDLE_ADOPT_with_t_NULL_returns) // no-srs
{
    // arrange
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(node_dispose);
    ASSERT_IS_NOT_NULL(node);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_ADOPT(NODE)(NULL, node);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_FREE(NODE)(node);
}

TEST_FUNCTION(UNIQUE_THANDLE_INSPECT_returns_the_object_with_a_reference_count_of_1) // no-srs
{
    // arrange
    UNIQUE_THANDLE(NODE) source = { { NULL } };
    UNIQUE_THANDLE(NODE) destination = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, source));
    const THANDLE_WRAPPER_TYPE_NAME(NODE)* created = UNIQUE_THANDLE_INSPECT(NODE)(source);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(destination, source);
    const THANDLE_WRAPPER_TYPE_NAME(NODE)* moved = UNIQUE_THANDLE_INSPECT(NODE)(destination);
    const THANDLE_WRAPPER_TYPE_NAME(NODE)* none = UNIQUE_THANDLE_INSPECT(NODE)(source);
    const THANDLE_WRAPPER_TYPE_NAME(NODE)* no_handle = UNIQUE_THANDLE_INSPECT(NODE)(NULL);

    // assert
    ASSERT_IS_NOT_NULL(created);
    ASSERT_ARE_EQUAL(void_ptr, created, moved);
    ASSERT_ARE_EQUAL(int32_t, 1, moved->refCount);
    ASSERT_IS_TRUE(moved->dispose == node_dispose);
    ASSERT_ARE_EQUAL(void_ptr, &moved->data, UNIQUE_THANDLE_GET_T(NODE)(destination));
    ASSERT_IS_NULL(none);
    ASSERT_IS_NULL(no_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(destination);
}

/* UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS */

/*Tests_SRS_UNIQUE_THANDLE_43_002: [ UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS shall use malloc_function, malloc_flex_function and free_function to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_MALLOC_allocates_with_the_malloc_function_of_the_type)
{
    // arrange
    STRICT_EXPECTED_CALL(type_malloc(IGNORED_ARG));

    // act
    TYPED* typed = UNIQUE_THANDLE_MALLOC(TYPED)(NULL);

    // assert
    ASSERT_IS_NOT_NULL(typed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_FREE(TYPED)(typed);
}

/*Tests_SRS_UNIQUE_THANDLE_43_002: [ UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS shall use malloc_function, malloc_flex_function and free_function to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_MALLOC_FLEX_allocates_with_the_malloc_flex_function_of_the_type)
{
    // arrange
    STRICT_EXPECTED_CALL(type_malloc_flex(IGNORED_ARG, 2, 4));

    // act
    TYPED* typed = UNIQUE_THANDLE_MALLOC_FLEX(TYPED)(NULL, 2, 4);

    // assert
    ASSERT_IS_NOT_NULL(typed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_FREE(TYPED)(typed);
}

/*Tests_SRS_UNIQUE_THANDLE_43_002: [ UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS shall use malloc_function, malloc_flex_function and free_function to allocate and free memory. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_frees_with_the_free_function_of_the_type)
{
    // arrange
    UNIQUE_THANDLE(TYPED) typed = { { NULL } };
    TYPED* object = UNIQUE_THANDLE_MALLOC(TYPED)(NULL);
    ASSERT_IS_NOT_NULL(object);
    UNIQUE_THANDLE_ADOPT(TYPED)(typed, object);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(type_free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_DISPOSE(TYPED)(typed);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(TYPED)(typed));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* UNIQUE_THANDLE_LL_TYPE_DEFINE */

TEST_FUNCTION(UNIQUE_THANDLE_LL_TYPE_DEFINE_names_the_functions_after_C) // no-srs
{
    // arrange
    UNIQUE_THANDLE(LL_STRUCT) source = { { NULL } };
    UNIQUE_THANDLE(LL_STRUCT) destination = { { NULL } };
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    LL_STRUCT* object = UNIQUE_THANDLE_MALLOC(LL_NAME)(NULL);
    ASSERT_IS_NOT_NULL(object);
    UNIQUE_THANDLE_ADOPT(LL_NAME)(source, object);

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(LL_NAME)(destination, source);
    LL_STRUCT* moved = UNIQUE_THANDLE_GET_T(LL_NAME)(destination);
    LL_STRUCT* left_in_source = UNIQUE_THANDLE_GET_T(LL_NAME)(source);
    UNIQUE_THANDLE_DISPOSE(LL_NAME)(destination);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, object, moved);
    ASSERT_IS_NULL(left_in_source);
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(LL_NAME)(destination));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* the ITEM and the list of the example in the requirements: UNIQUE_THANDLE(ITEM) is declared in a header and defined in another .c file */

TEST_FUNCTION(UNIQUE_THANDLE_item_moves_into_a_list_that_disposes_it) // no-srs
{
    // arrange
    UNIQUE_THANDLE(ITEM) item; /*not initialized: item_create puts a new item in it*/
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, IGNORED_ARG));
    LIST_HANDLE list = list_create(2);
    ASSERT_IS_NOT_NULL(list);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*item_create*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*list_destroy disposes the item*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*list_destroy frees the list*/

    // act
    int create_result = item_create(42, item);
    int value = item_get_value(item);
    int add_result = list_add(list, item);
    int value_after_add = item_get_value(item);
    UNIQUE_THANDLE_DISPOSE(ITEM)(item); /*item is NULL, the list owns the item*/
    uint32_t count = list_get_count(list);
    list_destroy(list);

    // assert
    ASSERT_ARE_EQUAL(int, 0, create_result);
    ASSERT_ARE_EQUAL(int, 42, value);
    ASSERT_ARE_EQUAL(int, 0, add_result);
    ASSERT_ARE_EQUAL(int, INT_MAX, value_after_add);
    ASSERT_ARE_EQUAL(uint32_t, 1, count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(UNIQUE_THANDLE_item_that_a_full_list_rejects_stays_with_its_owner) // no-srs
{
    // arrange
    UNIQUE_THANDLE(ITEM) first = { { NULL } };
    UNIQUE_THANDLE(ITEM) second = { { NULL } };
    LIST_HANDLE list = list_create(1);
    ASSERT_IS_NOT_NULL(list);
    ASSERT_ARE_EQUAL(int, 0, item_create(1, first));
    ASSERT_ARE_EQUAL(int, 0, list_add(list, first));
    ASSERT_ARE_EQUAL(int, 0, item_create(2, second));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*the owner of second disposes it*/

    // act
    int add_result = list_add(list, second);
    int value = item_get_value(second);
    UNIQUE_THANDLE_DISPOSE(ITEM)(second);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, add_result);
    ASSERT_ARE_EQUAL(int, 2, value);
    ASSERT_ARE_EQUAL(int, INT_MAX, item_get_value(second));
    ASSERT_ARE_EQUAL(uint32_t, 1, list_get_count(list));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    list_destroy(list);
}

/* UNIQUE_THANDLE_INITIALIZE_MOVE */

/*Tests_SRS_UNIQUE_THANDLE_43_008: [ If t1 is NULL then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_with_t1_NULL_returns)
{
    // arrange
    UNIQUE_THANDLE(NODE) source = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, source));
    NODE* original = UNIQUE_THANDLE_GET_T(NODE)(source);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(NULL, source);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, original, UNIQUE_THANDLE_GET_T(NODE)(source));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(source);
}

/*Tests_SRS_UNIQUE_THANDLE_43_009: [ If t2 is NULL then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_with_t2_NULL_returns)
{
    // arrange
    HOLDER* holder = holder_create_uninitialized();
    ASSERT_IS_NOT_NULL(holder);

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(holder->field, NULL);

    // assert
    ASSERT_IS_TRUE(holder_is_uninitialized(holder));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    real_gballoc_hl_free(holder);
}

/*Tests_SRS_UNIQUE_THANDLE_43_010: [ If t1 is equal to t2 then UNIQUE_THANDLE_INITIALIZE_MOVE shall return. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_with_t1_equal_to_t2_returns)
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, handle));
    NODE* original = UNIQUE_THANDLE_GET_T(NODE)(handle);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(handle, handle);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, original, UNIQUE_THANDLE_GET_T(NODE)(handle));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);
}

/*Tests_SRS_UNIQUE_THANDLE_43_011: [ UNIQUE_THANDLE_INITIALIZE_MOVE shall store *t2 in *t1 and set *t2 to NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_moves_t2_into_t1_that_is_not_initialized)
{
    // arrange
    UNIQUE_THANDLE(NODE) source = { { NULL } };
    HOLDER* holder = holder_create_uninitialized();
    ASSERT_IS_NOT_NULL(holder);
    ASSERT_ARE_EQUAL(int, 0, node_create(1, source));
    NODE* original = UNIQUE_THANDLE_GET_T(NODE)(source);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(holder->field, source);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, original, UNIQUE_THANDLE_GET_T(NODE)(holder->field));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(source));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(holder->field);
    real_gballoc_hl_free(holder);
}

/*Tests_SRS_UNIQUE_THANDLE_43_011: [ UNIQUE_THANDLE_INITIALIZE_MOVE shall store *t2 in *t1 and set *t2 to NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_moves_t2_into_NULL_t1)
{
    // arrange
    UNIQUE_THANDLE(NODE) destination = { { NULL } };
    UNIQUE_THANDLE(NODE) source = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, source));
    NODE* original = UNIQUE_THANDLE_GET_T(NODE)(source);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(destination, source);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, original, UNIQUE_THANDLE_GET_T(NODE)(destination));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(source));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(destination);
}

/*Tests_SRS_UNIQUE_THANDLE_43_011: [ UNIQUE_THANDLE_INITIALIZE_MOVE shall store *t2 in *t1 and set *t2 to NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_does_not_dispose_a_handle_held_by_t1)
{
    // arrange
    UNIQUE_THANDLE(TYPED) destination = { { NULL } };
    UNIQUE_THANDLE(TYPED) source = { { NULL } };
    TYPED* held = UNIQUE_THANDLE_MALLOC(TYPED)(typed_dispose);
    ASSERT_IS_NOT_NULL(held);
    held->value = 1;
    UNIQUE_THANDLE_ADOPT(TYPED)(destination, held);
    UNIQUE_THANDLE(TYPED) copy = { *destination }; /*an explicit copy keeps the object of destination reachable after destination is overwritten*/
    TYPED* moved = UNIQUE_THANDLE_MALLOC(TYPED)(typed_dispose);
    ASSERT_IS_NOT_NULL(moved);
    moved->value = 2;
    UNIQUE_THANDLE_ADOPT(TYPED)(source, moved);
    umock_c_reset_all_calls();

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(TYPED)(destination, source);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, moved, UNIQUE_THANDLE_GET_T(TYPED)(destination));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(TYPED)(source));
    ASSERT_ARE_EQUAL(void_ptr, held, UNIQUE_THANDLE_GET_T(TYPED)(copy));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(TYPED)(destination);
    UNIQUE_THANDLE_DISPOSE(TYPED)(copy);
}

/*Tests_SRS_UNIQUE_THANDLE_43_011: [ UNIQUE_THANDLE_INITIALIZE_MOVE shall store *t2 in *t1 and set *t2 to NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_INITIALIZE_MOVE_of_a_NULL_handle_sets_t1_to_NULL)
{
    // arrange
    UNIQUE_THANDLE(NODE) source = { { NULL } };
    HOLDER* holder = holder_create_uninitialized();
    ASSERT_IS_NOT_NULL(holder);

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(holder->field, source);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(holder->field));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(source));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    real_gballoc_hl_free(holder);
}

/* UNIQUE_THANDLE_DISPOSE */

/*Tests_SRS_UNIQUE_THANDLE_43_012: [ If t is NULL then UNIQUE_THANDLE_DISPOSE shall return. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_with_t_NULL_returns)
{
    // arrange

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_013: [ If *t is NULL then UNIQUE_THANDLE_DISPOSE shall return. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_with_NULL_handle_returns)
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(handle));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_001: [ UNIQUE_THANDLE_TYPE_DEFINE shall use malloc, malloc_flex and free to allocate and free memory. ]*/
/*Tests_SRS_UNIQUE_THANDLE_43_014: [ UNIQUE_THANDLE_DISPOSE shall set *t to NULL. ]*/
/*Tests_SRS_UNIQUE_THANDLE_43_015: [ UNIQUE_THANDLE_DISPOSE shall then call the dispose function passed when the handle was created, if it is not NULL. ]*/
/*Tests_SRS_UNIQUE_THANDLE_43_016: [ UNIQUE_THANDLE_DISPOSE shall free the memory of the handle. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_sets_t_to_NULL_then_calls_dispose_and_frees)
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, handle));
    g.observed_handle = &handle;
    g.object_at_dispose = UNIQUE_THANDLE_GET_T(NODE)(handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_object_disposed(1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(handle));
    ASSERT_IS_NULL(g.object_at_dispose);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_015: [ UNIQUE_THANDLE_DISPOSE shall then call the dispose function passed when the handle was created, if it is not NULL. ]*/
/*Tests_SRS_UNIQUE_THANDLE_43_016: [ UNIQUE_THANDLE_DISPOSE shall free the memory of the handle. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_without_dispose_function_frees)
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };
    NODE* node = UNIQUE_THANDLE_MALLOC(NODE)(NULL);
    ASSERT_IS_NOT_NULL(node);
    UNIQUE_THANDLE_ADOPT(NODE)(handle, node);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(handle));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_015: [ UNIQUE_THANDLE_DISPOSE shall then call the dispose function passed when the handle was created, if it is not NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_disposes_a_chain_of_handles)
{
    // arrange
    UNIQUE_THANDLE(NODE) head = { { NULL } };
    UNIQUE_THANDLE(NODE) second = { { NULL } };
    UNIQUE_THANDLE(NODE) third = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, head));
    ASSERT_ARE_EQUAL(int, 0, node_create(2, second));
    ASSERT_ARE_EQUAL(int, 0, node_create(3, third));
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(UNIQUE_THANDLE_GET_T(NODE)(second)->next, third);
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(UNIQUE_THANDLE_GET_T(NODE)(head)->next, second);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_object_disposed(1));
    STRICT_EXPECTED_CALL(test_object_disposed(2));
    STRICT_EXPECTED_CALL(test_object_disposed(3));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(head);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(head));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(second));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(third));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UNIQUE_THANDLE_43_013: [ If *t is NULL then UNIQUE_THANDLE_DISPOSE shall return. ]*/
/*Tests_SRS_UNIQUE_THANDLE_43_014: [ UNIQUE_THANDLE_DISPOSE shall set *t to NULL. ]*/
TEST_FUNCTION(UNIQUE_THANDLE_DISPOSE_from_the_dispose_function_of_the_handle_being_disposed_returns)
{
    // arrange
    UNIQUE_THANDLE(NODE) handle = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, handle));
    g.observed_handle = &handle;
    g.object_at_dispose = UNIQUE_THANDLE_GET_T(NODE)(handle);
    g.reentrant_handle = &handle;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_object_disposed(1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_DISPOSE(NODE)(handle);

    // assert
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(handle));
    ASSERT_IS_NULL(g.object_at_dispose);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* replacing an object with a handle that the object owns */

TEST_FUNCTION(UNIQUE_THANDLE_pops_the_head_of_a_list_by_moving_its_next_out_before_disposing_it) // no-srs
{
    // arrange
    UNIQUE_THANDLE(NODE) head = { { NULL } };
    UNIQUE_THANDLE(NODE) second = { { NULL } };
    UNIQUE_THANDLE(NODE) next = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, head));
    ASSERT_ARE_EQUAL(int, 0, node_create(2, second));
    NODE* original_second = UNIQUE_THANDLE_GET_T(NODE)(second);
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(UNIQUE_THANDLE_GET_T(NODE)(head)->next, second);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_object_disposed(1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(next, UNIQUE_THANDLE_GET_T(NODE)(head)->next);
    UNIQUE_THANDLE_DISPOSE(NODE)(head);
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(head, next);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, original_second, UNIQUE_THANDLE_GET_T(NODE)(head));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(next));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(UNIQUE_THANDLE_GET_T(NODE)(head)->next));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    UNIQUE_THANDLE_DISPOSE(NODE)(head);
}

/* handles with static storage duration */

TEST_FUNCTION(UNIQUE_THANDLE_moves_into_and_disposes_handles_with_static_storage_duration) // no-srs
{
    // arrange
    static UNIQUE_THANDLE(NODE) function_scope_handle = { { NULL } };
    UNIQUE_THANDLE(NODE) source_1 = { { NULL } };
    UNIQUE_THANDLE(NODE) source_2 = { { NULL } };
    UNIQUE_THANDLE(NODE) source_3 = { { NULL } };
    ASSERT_ARE_EQUAL(int, 0, node_create(1, source_1));
    ASSERT_ARE_EQUAL(int, 0, node_create(2, source_2));
    ASSERT_ARE_EQUAL(int, 0, node_create(3, source_3));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_object_disposed(1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_object_disposed(2));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_object_disposed(3));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(g_file_scope_handle, source_1);
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(g_file_scope_handle_without_initializer, source_2);
    UNIQUE_THANDLE_INITIALIZE_MOVE(NODE)(function_scope_handle, source_3);
    int value_1 = UNIQUE_THANDLE_GET_T(NODE)(g_file_scope_handle)->value;
    int value_2 = UNIQUE_THANDLE_GET_T(NODE)(g_file_scope_handle_without_initializer)->value;
    int value_3 = UNIQUE_THANDLE_GET_T(NODE)(function_scope_handle)->value;
    UNIQUE_THANDLE_DISPOSE(NODE)(g_file_scope_handle);
    UNIQUE_THANDLE_DISPOSE(NODE)(g_file_scope_handle_without_initializer);
    UNIQUE_THANDLE_DISPOSE(NODE)(function_scope_handle);

    // assert
    ASSERT_ARE_EQUAL(int, 1, value_1);
    ASSERT_ARE_EQUAL(int, 2, value_2);
    ASSERT_ARE_EQUAL(int, 3, value_3);
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(g_file_scope_handle));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(g_file_scope_handle_without_initializer));
    ASSERT_IS_NULL(UNIQUE_THANDLE_GET_T(NODE)(function_scope_handle));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
