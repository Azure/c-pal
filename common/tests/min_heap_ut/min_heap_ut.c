// Copyright (C) Microsoft Corporation. All rights reserved.

#include "min_heap_ut_pch.h"

#define TEST_MAX_ENTRIES 256
#define TEST_MANY_ENTRIES 200

#define TEST_COUNT(array) ((uint32_t)MU_COUNT_ARRAY_ITEMS(array))

// mirrors the private MIN_HEAP struct of min_heap.c
// only the tests that need a reserved count of UINT32_MAX or a capacity above UINT32_MAX / 2 use it: reaching those
// through the API would take billions of min_heap_reserve calls and an array of entries of 32 GB
typedef struct TEST_MIN_HEAP_LAYOUT_TAG
{
    MIN_HEAP_ENTRY** entries;
    uint32_t count;
    uint32_t reserved_count;
    uint32_t capacity;
} TEST_MIN_HEAP_LAYOUT;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

// fills permutation with 0..count-1 in a random order, the fixed seed makes a failure reproducible
static void test_get_random_permutation(uint32_t* permutation, uint32_t count, uint32_t seed)
{
    uint32_t random_state = seed;

    for (uint32_t i = 0; i < count; i++)
    {
        permutation[i] = i;
    }

    // Fisher-Yates shuffle with xorshift32
    for (uint32_t i = count - 1; i > 0; i--)
    {
        random_state ^= random_state << 13;
        random_state ^= random_state >> 17;
        random_state ^= random_state << 5;
        uint32_t j = random_state % (i + 1);

        uint32_t temp = permutation[i];
        permutation[i] = permutation[j];
        permutation[j] = temp;
    }
}

static void test_sort_keys(uint64_t* keys, uint32_t count)
{
    for (uint32_t i = 1; i < count; i++)
    {
        uint64_t key = keys[i];
        uint32_t j = i;
        while ((j > 0) && (keys[j - 1] > key))
        {
            keys[j] = keys[j - 1];
            j--;
        }
        keys[j] = key;
    }
}

// creates a heap and reserves room for reserved_count entries, the array of entries grows to 16 entries on the first
// reserve and doubles each time the reserved count reaches its capacity
static MIN_HEAP_HANDLE test_create_heap_with_reserved_room(uint32_t reserved_count)
{
    uint32_t capacity = 0;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    for (uint32_t i = 0; i < reserved_count; i++)
    {
        if (i < capacity)
        {
            /* do nothing, the array of entries has room for one more reserved entry */
        }
        else
        {
            if (capacity == 0)
            {
                capacity = 16;
                STRICT_EXPECTED_CALL(realloc_2(NULL, capacity, sizeof(MIN_HEAP_ENTRY*)));
            }
            else
            {
                capacity *= 2;
                STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, capacity, sizeof(MIN_HEAP_ENTRY*)));
            }
        }
    }

    MIN_HEAP_HANDLE result = min_heap_create();
    ASSERT_IS_NOT_NULL(result);

    for (uint32_t i = 0; i < reserved_count; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, min_heap_reserve(result));
    }

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    return result;
}

static void test_init_entries(MIN_HEAP_ENTRY* entries, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        min_heap_entry_init(&entries[i]);
    }
}

// initializes and inserts entries[i] with keys[i] for each i below count
static void test_insert_entries(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entries, const uint64_t* keys, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        min_heap_entry_init(&entries[i]);
        min_heap_insert(min_heap, &entries[i], keys[i]);
        ASSERT_ARE_NOT_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[i].index);
    }
}

// rebuilds the heap from the index of the entries (all entries in the heap must be in entries), checks that the
// indexes are unique and contiguous, that no entry has a key lower than the key of its parent, and that
// min_heap_peek returns the entry at index 0
static void test_assert_heap_is_valid(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entries, uint32_t entry_count)
{
    MIN_HEAP_ENTRY* positions[TEST_MAX_ENTRIES] = { NULL };
    uint32_t count_in_heap = 0;

    ASSERT_IS_TRUE(entry_count <= TEST_MAX_ENTRIES);

    for (uint32_t i = 0; i < entry_count; i++)
    {
        uint32_t index = entries[i].index;
        if (index == MIN_HEAP_ENTRY_NOT_IN_HEAP)
        {
            /* do nothing, the entry is not in the heap */
        }
        else
        {
            ASSERT_IS_TRUE(index < entry_count, "entry %" PRIu32 " has index %" PRIu32 "", i, index);
            ASSERT_IS_NULL(positions[index], "more than one entry has index %" PRIu32 "", index);
            positions[index] = &entries[i];
            count_in_heap++;
        }
    }

    for (uint32_t i = 0; i < count_in_heap; i++)
    {
        ASSERT_IS_NOT_NULL(positions[i], "no entry has index %" PRIu32 " in a heap of %" PRIu32 " entries", i, count_in_heap);
        if (i == 0)
        {
            /* do nothing, the root has no parent */
        }
        else
        {
            uint32_t parent = (i - 1) / 2;
            ASSERT_IS_TRUE(positions[parent]->key <= positions[i]->key, "entry at index %" PRIu32 " has key %" PRIu64 " lower than the key %" PRIu64 " of its parent", i, positions[i]->key, positions[parent]->key);
        }
    }

    ASSERT_ARE_EQUAL(void_ptr, (count_in_heap == 0) ? NULL : positions[0], min_heap_peek(min_heap));
}

// removes the entry returned by min_heap_peek until the heap is empty and checks that the keys come out as expected_keys
static void test_drain_and_assert_keys(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entries, uint32_t entry_count, const uint64_t* expected_keys, uint32_t expected_count)
{
    for (uint32_t i = 0; i < expected_count; i++)
    {
        MIN_HEAP_ENTRY* top = min_heap_peek(min_heap);
        ASSERT_IS_NOT_NULL(top, "heap is empty after %" PRIu32 " of %" PRIu32 " entries", i, expected_count);
        ASSERT_ARE_EQUAL(uint64_t, expected_keys[i], top->key, "unexpected key for entry %" PRIu32 "", i);
        ASSERT_ARE_EQUAL(uint32_t, 0, top->index);

        min_heap_remove(min_heap, top);

        ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, top->index);
        ASSERT_ARE_EQUAL(uint64_t, expected_keys[i], top->key);
        test_assert_heap_is_valid(min_heap, entries, entry_count);
    }

    ASSERT_IS_NULL(min_heap_peek(min_heap));
}

static TEST_MIN_HEAP_LAYOUT* test_get_layout_of_heap_with_room_for_1_entry(MIN_HEAP_HANDLE min_heap)
{
    TEST_MIN_HEAP_LAYOUT* result = (TEST_MIN_HEAP_LAYOUT*)(void*)min_heap;

    // fails if TEST_MIN_HEAP_LAYOUT no longer matches the private struct of min_heap.c
    ASSERT_IS_NOT_NULL(result->entries);
    ASSERT_ARE_EQUAL(uint32_t, 0, result->count);
    ASSERT_ARE_EQUAL(uint32_t, 1, result->reserved_count);
    ASSERT_ARE_EQUAL(uint32_t, 16, result->capacity);

    return result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* min_heap_create */

/*Tests_SRS_MIN_HEAP_42_001: [ min_heap_create shall allocate a new heap by calling malloc. ]*/
/*Tests_SRS_MIN_HEAP_42_039: [ min_heap_create shall succeed and return the heap. ]*/
TEST_FUNCTION(min_heap_create_succeeds)
{
    // arrange
    void* allocated_heap = NULL;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_heap);

    // act
    MIN_HEAP_HANDLE min_heap = min_heap_create();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(min_heap);
    ASSERT_ARE_EQUAL(void_ptr, allocated_heap, min_heap);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_038: [ min_heap_create shall set the heap to have no entries, no reserved room and no array of entries. ]*/
TEST_FUNCTION(min_heap_create_sets_the_heap_to_have_no_entries_and_no_reserved_room)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    MIN_HEAP_HANDLE min_heap = min_heap_create();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(min_heap);
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // there is no reserved room, so the entry is not inserted
    min_heap_insert(min_heap, &entry, 1);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_038: [ min_heap_create shall set the heap to have no entries, no reserved room and no array of entries. ]*/
TEST_FUNCTION(min_heap_create_sets_the_heap_to_have_no_array_of_entries)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    MIN_HEAP_HANDLE min_heap = min_heap_create();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(min_heap);

    // the first reserve allocates the array of entries from NULL
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)));
    ASSERT_ARE_EQUAL(int, 0, min_heap_reserve(min_heap));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_002: [ If any error occurs, min_heap_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_malloc_fails_min_heap_create_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    // act
    MIN_HEAP_HANDLE min_heap = min_heap_create();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(min_heap);
}

/* min_heap_destroy */

/*Tests_SRS_MIN_HEAP_42_003: [ If min_heap is NULL, min_heap_destroy shall return. ]*/
TEST_FUNCTION(min_heap_destroy_with_NULL_min_heap_returns)
{
    // arrange

    // act
    min_heap_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MIN_HEAP_42_004: [ If the heap has an array of entries, min_heap_destroy shall free it. ]*/
/*Tests_SRS_MIN_HEAP_42_005: [ min_heap_destroy shall free min_heap. ]*/
TEST_FUNCTION(min_heap_destroy_frees_the_array_of_entries_and_the_heap)
{
    // arrange
    static const uint64_t keys[] = { 30, 10, 20 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];
    void* entries_array = NULL;

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)))
        .CaptureReturn(&entries_array);
    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        ASSERT_ARE_EQUAL(int, 0, min_heap_reserve(min_heap));
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(entries_array);
    umock_c_reset_all_calls();

    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    STRICT_EXPECTED_CALL(free(entries_array));
    STRICT_EXPECTED_CALL(free(min_heap));

    // act
    min_heap_destroy(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MIN_HEAP_42_004: [ If the heap has an array of entries, min_heap_destroy shall free it. ]*/
/*Tests_SRS_MIN_HEAP_42_005: [ min_heap_destroy shall free min_heap. ]*/
TEST_FUNCTION(min_heap_destroy_of_a_heap_with_no_array_of_entries_frees_the_heap)
{
    // arrange
    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(free(min_heap));

    // act
    min_heap_destroy(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MIN_HEAP_42_004: [ If the heap has an array of entries, min_heap_destroy shall free it. ]*/
/*Tests_SRS_MIN_HEAP_42_005: [ min_heap_destroy shall free min_heap. ]*/
TEST_FUNCTION(min_heap_destroy_does_not_touch_the_entries_still_in_the_heap)
{
    // arrange
    static const uint64_t keys[] = { 30, 10, 20 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];
    MIN_HEAP_ENTRY entries_before_destroy[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        entries_before_destroy[i] = entries[i];
    }

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(min_heap));

    // act
    min_heap_destroy(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        ASSERT_ARE_EQUAL(uint64_t, entries_before_destroy[i].key, entries[i].key);
        ASSERT_ARE_EQUAL(uint32_t, entries_before_destroy[i].index, entries[i].index);
    }
}

/* min_heap_reserve */

/*Tests_SRS_MIN_HEAP_42_006: [ If min_heap is NULL, min_heap_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(min_heap_reserve_with_NULL_min_heap_fails)
{
    // arrange

    // act
    int result = min_heap_reserve(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_MIN_HEAP_42_007: [ If the reserved count is UINT32_MAX, min_heap_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(min_heap_reserve_when_the_reserved_count_is_UINT32_MAX_fails)
{
    // arrange
    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    TEST_MIN_HEAP_LAYOUT* layout = test_get_layout_of_heap_with_room_for_1_entry(min_heap);
    layout->reserved_count = UINT32_MAX;
    layout->capacity = UINT32_MAX;

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, layout->reserved_count);
    ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, layout->capacity);

    // cleanup
    layout->reserved_count = 1;
    layout->capacity = 16;
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_009: [ min_heap_reserve shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at UINT32_MAX otherwise. ]*/
/*Tests_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
/*Tests_SRS_MIN_HEAP_42_012: [ min_heap_reserve shall increment the reserved count. ]*/
/*Tests_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(min_heap_reserve_the_first_time_grows_the_array_of_entries_to_16_entries)
{
    // arrange
    MIN_HEAP_ENTRY entries[2];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)));

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // there is room for exactly 1 entry
    min_heap_insert(min_heap, &entries[0], 1);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    min_heap_insert(min_heap, &entries[1], 2);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[1].index);
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_012: [ min_heap_reserve shall increment the reserved count. ]*/
/*Tests_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(min_heap_reserve_does_not_grow_the_array_of_entries_while_the_reserved_count_is_below_the_capacity)
{
    // arrange
    MIN_HEAP_ENTRY entries[17];
    uint64_t keys[17];
    int results[15];

    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        keys[i] = 17 - i;
    }
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    // act
    for (uint32_t i = 0; i < TEST_COUNT(results); i++)
    {
        results[i] = min_heap_reserve(min_heap);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(results); i++)
    {
        ASSERT_ARE_EQUAL(int, 0, results[i]);
    }

    // there is room for exactly 16 entries
    test_insert_entries(min_heap, entries, keys, 16);
    min_heap_insert(min_heap, &entries[16], keys[16]);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[16].index);
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_009: [ min_heap_reserve shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at UINT32_MAX otherwise. ]*/
/*Tests_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
/*Tests_SRS_MIN_HEAP_42_012: [ min_heap_reserve shall increment the reserved count. ]*/
/*Tests_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(min_heap_reserve_the_17th_time_doubles_the_array_of_entries_to_32_entries)
{
    // arrange
    MIN_HEAP_ENTRY entries[18];
    uint64_t keys[17];
    uint64_t expected_keys[17];

    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        keys[i] = 16 - i;
        expected_keys[i] = i;
    }
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(16);
    test_insert_entries(min_heap, entries, keys, 16);

    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, 32, sizeof(MIN_HEAP_ENTRY*)));

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // there is room for exactly 17 entries, and the entries inserted before the array grew are still in the heap
    min_heap_insert(min_heap, &entries[16], keys[16]);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[16].index);
    min_heap_insert(min_heap, &entries[17], 100);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[17].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_009: [ min_heap_reserve shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at UINT32_MAX otherwise. ]*/
/*Tests_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
/*Tests_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(min_heap_reserve_grows_the_array_of_entries_only_when_the_reserved_count_reaches_the_capacity)
{
    // arrange
    int results[33];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)));
    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, 32, sizeof(MIN_HEAP_ENTRY*)));
    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, 64, sizeof(MIN_HEAP_ENTRY*)));

    // act
    for (uint32_t i = 0; i < TEST_COUNT(results); i++)
    {
        results[i] = min_heap_reserve(min_heap);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(results); i++)
    {
        ASSERT_ARE_EQUAL(int, 0, results[i]);
    }

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_009: [ min_heap_reserve shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at UINT32_MAX otherwise. ]*/
/*Tests_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
/*Tests_SRS_MIN_HEAP_42_042: [ If realloc_2 fails, min_heap_reserve shall fail and return a non-zero value. ]*/
PARAMETERIZED_TEST_FUNCTION(min_heap_reserve_with_a_large_capacity_doubles_it_capped_at_UINT32_MAX,
    ARGS(uint32_t, capacity, uint32_t, expected_new_capacity),
    CASE((UINT32_MAX / 2, UINT32_MAX - 1), doubles_UINT32_MAX_divided_by_2),
    CASE(((UINT32_MAX / 2) + 1, UINT32_MAX), caps_UINT32_MAX_divided_by_2_plus_1),
    CASE((UINT32_MAX - 1, UINT32_MAX), caps_UINT32_MAX_minus_1))
{
    // arrange
    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    TEST_MIN_HEAP_LAYOUT* layout = test_get_layout_of_heap_with_room_for_1_entry(min_heap);
    layout->reserved_count = capacity;
    layout->capacity = capacity;

    // fail realloc_2 instead of allocating up to 32 GB
    STRICT_EXPECTED_CALL(realloc_2(layout->entries, expected_new_capacity, sizeof(MIN_HEAP_ENTRY*)))
        .SetReturn(NULL);

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, capacity, layout->reserved_count);
    ASSERT_ARE_EQUAL(uint32_t, capacity, layout->capacity);

    // cleanup
    layout->reserved_count = 1;
    layout->capacity = 16;
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_042: [ If realloc_2 fails, min_heap_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_realloc_2_fails_min_heap_reserve_fails)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)))
        .SetReturn(NULL);

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // the reserved count did not change, so the entry is not inserted
    min_heap_insert(min_heap, &entry, 1);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
/*Tests_SRS_MIN_HEAP_42_012: [ min_heap_reserve shall increment the reserved count. ]*/
/*Tests_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(min_heap_reserve_after_realloc_2_failed_grows_the_array_of_entries_again)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)))
        .SetReturn(NULL);
    ASSERT_ARE_NOT_EQUAL(int, 0, min_heap_reserve(min_heap));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)));

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    min_heap_insert(min_heap, &entry, 1);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);
    ASSERT_ARE_EQUAL(void_ptr, &entry, min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
/*Tests_SRS_MIN_HEAP_42_042: [ If realloc_2 fails, min_heap_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_realloc_2_fails_to_grow_the_array_of_entries_min_heap_reserve_fails_and_keeps_the_entries)
{
    // arrange
    MIN_HEAP_ENTRY entries[17];
    uint64_t keys[17];
    uint64_t expected_keys[16];

    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        // 7 and 17 are coprime, so the keys are 0..16 in a scrambled order
        keys[i] = (i * 7) % 17;
    }
    for (uint32_t i = 0; i < TEST_COUNT(expected_keys); i++)
    {
        expected_keys[i] = keys[i];
    }
    test_sort_keys(expected_keys, TEST_COUNT(expected_keys));
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(16);
    test_insert_entries(min_heap, entries, keys, 16);

    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, 32, sizeof(MIN_HEAP_ENTRY*)))
        .SetReturn(NULL);

    // act
    int result = min_heap_reserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // the reserved count did not change, so the 17th entry is not inserted
    min_heap_insert(min_heap, &entries[16], keys[16]);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[16].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/* min_heap_unreserve */

/*Tests_SRS_MIN_HEAP_42_013: [ If min_heap is NULL, min_heap_unreserve shall return. ]*/
TEST_FUNCTION(min_heap_unreserve_with_NULL_min_heap_returns)
{
    // arrange

    // act
    min_heap_unreserve(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MIN_HEAP_42_014: [ If the heap has as many entries as the reserved count, min_heap_unreserve shall return. ]*/
TEST_FUNCTION(min_heap_unreserve_with_no_reserved_room_returns)
{
    // arrange
    MIN_HEAP_ENTRY entries[2];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    // act
    min_heap_unreserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // the reserved count is still 0, so 1 reserve gives room for exactly 1 entry
    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(MIN_HEAP_ENTRY*)));
    ASSERT_ARE_EQUAL(int, 0, min_heap_reserve(min_heap));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    min_heap_insert(min_heap, &entries[0], 1);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    min_heap_insert(min_heap, &entries[1], 2);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[1].index);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_014: [ If the heap has as many entries as the reserved count, min_heap_unreserve shall return. ]*/
TEST_FUNCTION(min_heap_unreserve_when_all_the_reserved_room_is_used_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    static const uint64_t expected_keys[] = { 10, 30 };
    MIN_HEAP_ENTRY entries[3];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(2);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_unreserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // the reserved count is still 2, so once an entry is removed another one can be inserted
    min_heap_remove(min_heap, &entries[1]);
    min_heap_insert(min_heap, &entries[2], 30);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[2].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_015: [ min_heap_unreserve shall decrement the reserved count. ]*/
TEST_FUNCTION(min_heap_unreserve_gives_back_the_room_reserved_for_one_entry)
{
    // arrange
    static const uint64_t keys[] = { 10 };
    MIN_HEAP_ENTRY entries[2];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(2);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_unreserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // the reserved count is now 1, so the second entry is not inserted
    min_heap_insert(min_heap, &entries[1], 20);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[1].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), keys, TEST_COUNT(keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_015: [ min_heap_unreserve shall decrement the reserved count. ]*/
/*Tests_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
TEST_FUNCTION(min_heap_unreserve_does_not_shrink_the_array_of_entries)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    // act
    min_heap_unreserve(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // reserving again does not need to grow the array of entries
    ASSERT_ARE_EQUAL(int, 0, min_heap_reserve(min_heap));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    min_heap_insert(min_heap, &entry, 1);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);

    // cleanup
    min_heap_destroy(min_heap);
}

/* min_heap_entry_init */

/*Tests_SRS_MIN_HEAP_42_016: [ If entry is NULL, min_heap_entry_init shall return. ]*/
TEST_FUNCTION(min_heap_entry_init_with_NULL_entry_returns)
{
    // arrange

    // act
    min_heap_entry_init(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MIN_HEAP_42_017: [ min_heap_entry_init shall set the key of entry to 0. ]*/
/*Tests_SRS_MIN_HEAP_42_018: [ min_heap_entry_init shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_entry_init_sets_the_key_to_0_and_the_index_to_MIN_HEAP_ENTRY_NOT_IN_HEAP)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    entry.key = 42;
    entry.index = 7;

    // act
    min_heap_entry_init(&entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
}

/* min_heap_insert */

/*Tests_SRS_MIN_HEAP_42_019: [ If min_heap is NULL, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_with_NULL_min_heap_returns)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    // act
    min_heap_insert(NULL, &entry, 42);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
}

/*Tests_SRS_MIN_HEAP_42_020: [ If entry is NULL, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_with_NULL_entry_returns)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    // act
    min_heap_insert(min_heap, NULL, 42);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // the reserved room was not used
    min_heap_insert(min_heap, &entry, 42);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);
    ASSERT_ARE_EQUAL(void_ptr, &entry, min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_021: [ If the index of entry is not MIN_HEAP_ENTRY_NOT_IN_HEAP, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_with_entry_already_in_the_heap_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    static const uint64_t expected_keys[] = { 10, 20, 30 };
    MIN_HEAP_ENTRY entries[3];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(3);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[1], 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 20, entries[1].key);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[0], min_heap_peek(min_heap));
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // the reserved room for the third entry was not used
    min_heap_insert(min_heap, &entries[2], 30);
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[2].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_021: [ If the index of entry is not MIN_HEAP_ENTRY_NOT_IN_HEAP, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_with_entry_in_another_heap_returns)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE other_min_heap = test_create_heap_with_reserved_room(1);
    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);
    min_heap_insert(other_min_heap, &entry, 10);

    // act
    min_heap_insert(min_heap, &entry, 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(min_heap_peek(min_heap));
    ASSERT_ARE_EQUAL(uint64_t, 10, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);
    ASSERT_ARE_EQUAL(void_ptr, &entry, min_heap_peek(other_min_heap));

    // cleanup
    min_heap_destroy(min_heap);
    min_heap_destroy(other_min_heap);
}

/*Tests_SRS_MIN_HEAP_42_022: [ If the heap has as many entries as the reserved count, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_with_no_reserved_room_returns)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(0);

    // act
    min_heap_insert(min_heap, &entry, 42);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_022: [ If the heap has as many entries as the reserved count, min_heap_insert shall return. ]*/
TEST_FUNCTION(min_heap_insert_when_all_the_reserved_room_is_used_returns)
{
    // arrange
    static const uint64_t keys[] = { 20, 10 };
    static const uint64_t expected_keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[3];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(2);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[2], 1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, entries[2].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[2].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[1], min_heap_peek(min_heap));
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_023: [ min_heap_insert shall set the key of entry to key. ]*/
/*Tests_SRS_MIN_HEAP_42_024: [ min_heap_insert shall add entry after the last entry of the heap. ]*/
TEST_FUNCTION(min_heap_insert_into_an_empty_heap_sets_the_key_and_puts_the_entry_at_index_0)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    // act
    min_heap_insert(min_heap, &entry, 42);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 42, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);
    ASSERT_ARE_EQUAL(void_ptr, &entry, min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_024: [ min_heap_insert shall add entry after the last entry of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_with_key_higher_than_the_parent_key_adds_the_entry_after_the_last_entry)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[3];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(3);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[2], 30);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 30, entries[2].key);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[2].index);
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_024: [ min_heap_insert shall add entry after the last entry of the heap. ]*/
TEST_FUNCTION(min_heap_insert_ascending_keys_keeps_each_entry_at_its_insert_position)
{
    // arrange
    MIN_HEAP_ENTRY entries[10];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(entries));

    // act
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        min_heap_insert(min_heap, &entries[i], i);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        ASSERT_ARE_EQUAL(uint64_t, i, entries[i].key);
        ASSERT_ARE_EQUAL(uint32_t, i, entries[i].index);
    }
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_with_lower_key_moves_entry_to_the_top)
{
    // arrange
    static const uint64_t keys[] = { 10, 20, 30 };
    MIN_HEAP_ENTRY entries[4];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(4);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[3], 5);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // added at index 3, swapped with its parent at index 1 (20), then with the root (10)
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[3].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[2].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[3], min_heap_peek(min_heap));
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_stops_swapping_when_the_parent_key_is_lower)
{
    // arrange
    static const uint64_t keys[] = { 10, 20, 30 };
    MIN_HEAP_ENTRY entries[4];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(4);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[3], 15);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // added at index 3, swapped with its parent at index 1 (20), not swapped with the root (10)
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[3].index);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[2].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[0], min_heap_peek(min_heap));
    test_assert_heap_is_valid(min_heap, entries, TEST_COUNT(entries));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_with_key_equal_to_the_parent_key_does_not_swap)
{
    // arrange
    static const uint64_t keys[] = { 10 };
    MIN_HEAP_ENTRY entries[2];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(2);
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_insert(min_heap, &entries[1], 10);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[0], min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_023: [ min_heap_insert shall set the key of entry to key. ]*/
/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_orders_0_and_UINT64_MAX_keys)
{
    // arrange
    static const uint64_t keys[] = { UINT64_MAX, 0, UINT64_MAX - 1, 1, UINT64_MAX, 0 };
    static const uint64_t expected_keys[] = { 0, 0, 1, UINT64_MAX - 1, UINT64_MAX, UINT64_MAX };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));

    // act
    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        min_heap_insert(min_heap, &entries[i], keys[i]);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(keys); i++)
    {
        ASSERT_ARE_EQUAL(uint64_t, keys[i], entries[i].key);
    }
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_023: [ min_heap_insert shall set the key of entry to key. ]*/
/*Tests_SRS_MIN_HEAP_42_024: [ min_heap_insert shall add entry after the last entry of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
/*Tests_SRS_MIN_HEAP_42_037: [ Otherwise, min_heap_peek shall return the entry with the lowest key. ]*/
TEST_FUNCTION(min_heap_insert_many_shuffled_keys_with_duplicates_then_draining_returns_the_keys_in_ascending_order)
{
    // arrange
    MIN_HEAP_ENTRY entries[TEST_MANY_ENTRIES];
    uint32_t permutation[TEST_MANY_ENTRIES];
    uint64_t keys[TEST_MANY_ENTRIES];
    uint64_t expected_keys[TEST_MANY_ENTRIES];

    test_get_random_permutation(permutation, TEST_MANY_ENTRIES, 0x2A2A2A2A);
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        // each key is used 3 times
        keys[i] = permutation[i] / 3;
        expected_keys[i] = i / 3;
    }
    test_init_entries(entries, TEST_MANY_ENTRIES);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_MANY_ENTRIES);

    // act
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        min_heap_insert(min_heap, &entries[i], keys[i]);
        test_assert_heap_is_valid(min_heap, entries, TEST_MANY_ENTRIES);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        ASSERT_ARE_EQUAL(uint64_t, keys[i], entries[i].key);
    }
    test_drain_and_assert_keys(min_heap, entries, TEST_MANY_ENTRIES, expected_keys, TEST_MANY_ENTRIES);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_insert_descending_keys_then_draining_returns_the_keys_in_ascending_order)
{
    // arrange
    MIN_HEAP_ENTRY entries[TEST_MANY_ENTRIES];
    uint64_t expected_keys[TEST_MANY_ENTRIES];

    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        expected_keys[i] = i + 1;
    }
    test_init_entries(entries, TEST_MANY_ENTRIES);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_MANY_ENTRIES);

    // act
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        min_heap_insert(min_heap, &entries[i], TEST_MANY_ENTRIES - i);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // every entry had the lowest key when it was inserted, so the last one is at the top
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[TEST_MANY_ENTRIES - 1].index);
    test_assert_heap_is_valid(min_heap, entries, TEST_MANY_ENTRIES);
    test_drain_and_assert_keys(min_heap, entries, TEST_MANY_ENTRIES, expected_keys, TEST_MANY_ENTRIES);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
/*Tests_SRS_MIN_HEAP_42_037: [ Otherwise, min_heap_peek shall return the entry with the lowest key. ]*/
TEST_FUNCTION(min_heap_insert_entries_with_the_same_key_keeps_the_first_one_at_the_top)
{
    // arrange
    MIN_HEAP_ENTRY entries[20];
    uint64_t expected_keys[20];

    for (uint32_t i = 0; i < TEST_COUNT(expected_keys); i++)
    {
        expected_keys[i] = 7;
    }
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(entries));

    // act
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        min_heap_insert(min_heap, &entries[i], 7);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, i, entries[i].index);
    }
    ASSERT_ARE_EQUAL(void_ptr, &entries[0], min_heap_peek(min_heap));
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/* min_heap_remove */

/*Tests_SRS_MIN_HEAP_42_026: [ If min_heap is NULL, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_NULL_min_heap_returns)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);
    min_heap_insert(min_heap, &entry, 10);

    // act
    min_heap_remove(NULL, &entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 10, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.index);
    ASSERT_ARE_EQUAL(void_ptr, &entry, min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_027: [ If entry is NULL, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_NULL_entry_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), keys, TEST_COUNT(keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_028: [ If entry is not in min_heap, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_entry_not_in_a_heap_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[3];
    test_init_entries(entries, TEST_COUNT(entries));

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(entries));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, &entries[2]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, entries[2].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[2].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), keys, TEST_COUNT(keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_028: [ If entry is not in min_heap, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_entry_of_another_heap_at_an_index_valid_in_this_heap_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20, 30 };
    static const uint64_t other_keys[] = { 1, 2, 3 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];
    MIN_HEAP_ENTRY other_entries[MU_COUNT_ARRAY_ITEMS(other_keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    MIN_HEAP_HANDLE other_min_heap = test_create_heap_with_reserved_room(TEST_COUNT(other_keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    test_insert_entries(other_min_heap, other_entries, other_keys, TEST_COUNT(other_keys));
    ASSERT_ARE_EQUAL(uint32_t, 1, other_entries[1].index);

    // act
    min_heap_remove(min_heap, &other_entries[1]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2, other_entries[1].key);
    ASSERT_ARE_EQUAL(uint32_t, 1, other_entries[1].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), keys, TEST_COUNT(keys));
    test_drain_and_assert_keys(other_min_heap, other_entries, TEST_COUNT(other_entries), other_keys, TEST_COUNT(other_keys));

    // cleanup
    min_heap_destroy(min_heap);
    min_heap_destroy(other_min_heap);
}

/*Tests_SRS_MIN_HEAP_42_028: [ If entry is not in min_heap, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_entry_of_another_heap_at_an_index_past_the_last_entry_of_this_heap_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    static const uint64_t other_keys[] = { 1, 2, 3, 4 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];
    MIN_HEAP_ENTRY other_entries[MU_COUNT_ARRAY_ITEMS(other_keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    MIN_HEAP_HANDLE other_min_heap = test_create_heap_with_reserved_room(TEST_COUNT(other_keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    test_insert_entries(other_min_heap, other_entries, other_keys, TEST_COUNT(other_keys));
    ASSERT_ARE_EQUAL(uint32_t, 3, other_entries[3].index);

    // act
    min_heap_remove(min_heap, &other_entries[3]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 4, other_entries[3].key);
    ASSERT_ARE_EQUAL(uint32_t, 3, other_entries[3].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), keys, TEST_COUNT(keys));
    test_drain_and_assert_keys(other_min_heap, other_entries, TEST_COUNT(other_entries), other_keys, TEST_COUNT(other_keys));

    // cleanup
    min_heap_destroy(min_heap);
    min_heap_destroy(other_min_heap);
}

/*Tests_SRS_MIN_HEAP_42_028: [ If entry is not in min_heap, min_heap_remove shall return. ]*/
TEST_FUNCTION(min_heap_remove_with_entry_already_removed_returns)
{
    // arrange
    static const uint64_t keys[] = { 10, 20, 30 };
    static const uint64_t expected_keys[] = { 10, 30 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    min_heap_remove(min_heap, &entries[1]);

    // act
    min_heap_remove(min_heap, &entries[1]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 20, entries[1].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[1].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_029: [ min_heap_remove shall remove the last position of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_remove_the_only_entry_empties_the_heap)
{
    // arrange
    MIN_HEAP_ENTRY entry;
    min_heap_entry_init(&entry);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);
    min_heap_insert(min_heap, &entry, 42);

    // act
    min_heap_remove(min_heap, &entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 42, entry.key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entry.index);
    ASSERT_IS_NULL(min_heap_peek(min_heap));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_029: [ min_heap_remove shall remove the last position of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_remove_the_last_entry_does_not_move_the_other_entries)
{
    // arrange
    static const uint64_t keys[] = { 10, 20, 30 };
    static const uint64_t expected_keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, &entries[2]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 30, entries[2].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[2].index);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/
/*Tests_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
/*Tests_SRS_MIN_HEAP_42_032: [ min_heap_remove shall swap the moved entry with its child with the lowest key while its key is greater than the key of that child. ]*/
/*Tests_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_remove_the_root_moves_the_last_entry_down_to_the_child_with_the_lowest_key)
{
    // arrange
    static const uint64_t keys[] = { 1, 5, 3, 7, 9, 4 };
    static const uint64_t expected_keys[] = { 3, 4, 5, 7, 9 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, i, entries[i].index);
    }

    // act
    min_heap_remove(min_heap, &entries[0]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1, entries[0].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[0].index);

    // the last entry (4) moved to the root, then swapped with its child with the lowest key (3) and not with 5
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[2].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[5].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[3].index);
    ASSERT_ARE_EQUAL(uint32_t, 4, entries[4].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[2], min_heap_peek(min_heap));
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/
/*Tests_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
/*Tests_SRS_MIN_HEAP_42_032: [ min_heap_remove shall swap the moved entry with its child with the lowest key while its key is greater than the key of that child. ]*/
TEST_FUNCTION(min_heap_remove_a_middle_entry_moves_the_last_entry_down_more_than_one_level)
{
    // arrange
    static const uint64_t keys[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    static const uint64_t expected_keys[] = { 1, 3, 4, 5, 6, 7, 8, 9, 10 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, &entries[1]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2, entries[1].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[1].index);

    // the last entry (10) moved to index 1, then swapped with 4 at index 3 and with 8 at index 7
    ASSERT_ARE_EQUAL(uint32_t, 7, entries[9].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[3].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[7].index);
    ASSERT_ARE_EQUAL(uint32_t, 4, entries[4].index);
    ASSERT_ARE_EQUAL(uint32_t, 8, entries[8].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/
/*Tests_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
/*Tests_SRS_MIN_HEAP_42_033: [ min_heap_remove shall swap the moved entry with its parent while its key is lower than the key of its parent. ]*/
TEST_FUNCTION(min_heap_remove_a_middle_entry_moves_the_last_entry_up)
{
    // arrange
    static const uint64_t keys[] = { 1, 10, 2, 11, 12, 3, 4 };
    static const uint64_t expected_keys[] = { 1, 2, 3, 4, 10, 12 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    for (uint32_t i = 0; i < TEST_COUNT(entries); i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, i, entries[i].index);
    }

    // act
    min_heap_remove(min_heap, &entries[3]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 11, entries[3].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[3].index);

    // the last entry (4) moved to index 3 (no children), then swapped with its parent (10) and not with the root (1)
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[6].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/
/*Tests_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
TEST_FUNCTION(min_heap_remove_a_middle_entry_leaves_the_last_entry_at_the_position_of_the_entry)
{
    // arrange
    static const uint64_t keys[] = { 1, 2, 3, 4, 5 };
    static const uint64_t expected_keys[] = { 1, 2, 4, 5 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, &entries[2]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[2].index);

    // the last entry (5) moved to index 2, it has no children and its parent (1) has a lower key
    ASSERT_ARE_EQUAL(uint32_t, 2, entries[4].index);
    ASSERT_ARE_EQUAL(uint32_t, 0, entries[0].index);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[1].index);
    ASSERT_ARE_EQUAL(uint32_t, 3, entries[3].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_029: [ min_heap_remove shall remove the last position of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_remove_lets_the_entry_be_inserted_again)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    static const uint64_t expected_keys[] = { 20, 30 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    min_heap_remove(min_heap, &entries[0]);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 10, entries[0].key);
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[0].index);
    ASSERT_ARE_EQUAL(void_ptr, &entries[1], min_heap_peek(min_heap));

    // the room of the removed entry can be used to insert it again with another key
    min_heap_insert(min_heap, &entries[0], 30);
    ASSERT_ARE_EQUAL(uint32_t, 1, entries[0].index);
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_029: [ min_heap_remove shall remove the last position of the heap. ]*/
/*Tests_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/
/*Tests_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
/*Tests_SRS_MIN_HEAP_42_032: [ min_heap_remove shall swap the moved entry with its child with the lowest key while its key is greater than the key of that child. ]*/
/*Tests_SRS_MIN_HEAP_42_033: [ min_heap_remove shall swap the moved entry with its parent while its key is lower than the key of its parent. ]*/
/*Tests_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
TEST_FUNCTION(min_heap_remove_entries_in_random_order_keeps_the_heap_valid)
{
    // arrange
    MIN_HEAP_ENTRY entries[TEST_MANY_ENTRIES];
    uint32_t permutation[TEST_MANY_ENTRIES];
    uint32_t removal_order[TEST_MANY_ENTRIES];
    uint64_t keys[TEST_MANY_ENTRIES];
    uint64_t expected_keys[TEST_MANY_ENTRIES / 2];

    test_get_random_permutation(permutation, TEST_MANY_ENTRIES, 0x13579BDF);
    test_get_random_permutation(removal_order, TEST_MANY_ENTRIES, 0x2468ACE0);
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES; i++)
    {
        // each key is used 4 times
        keys[i] = permutation[i] / 4;
    }

    // the entries that are not removed come out in the order of their keys
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES / 2; i++)
    {
        expected_keys[i] = keys[removal_order[(TEST_MANY_ENTRIES / 2) + i]];
    }
    test_sort_keys(expected_keys, TEST_MANY_ENTRIES / 2);

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_MANY_ENTRIES);
    test_insert_entries(min_heap, entries, keys, TEST_MANY_ENTRIES);

    // act
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES / 2; i++)
    {
        min_heap_remove(min_heap, &entries[removal_order[i]]);
        test_assert_heap_is_valid(min_heap, entries, TEST_MANY_ENTRIES);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    for (uint32_t i = 0; i < TEST_MANY_ENTRIES / 2; i++)
    {
        ASSERT_ARE_EQUAL(uint64_t, keys[removal_order[i]], entries[removal_order[i]].key);
        ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, entries[removal_order[i]].index);
    }
    test_drain_and_assert_keys(min_heap, entries, TEST_MANY_ENTRIES, expected_keys, TEST_MANY_ENTRIES / 2);

    // cleanup
    min_heap_destroy(min_heap);
}

/* min_heap_peek */

/*Tests_SRS_MIN_HEAP_42_035: [ If min_heap is NULL, min_heap_peek shall return NULL. ]*/
TEST_FUNCTION(min_heap_peek_with_NULL_min_heap_returns_NULL)
{
    // arrange

    // act
    MIN_HEAP_ENTRY* result = min_heap_peek(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_MIN_HEAP_42_036: [ If the heap has no entries, min_heap_peek shall return NULL. ]*/
TEST_FUNCTION(min_heap_peek_on_a_heap_with_reserved_room_and_no_entries_returns_NULL)
{
    // arrange
    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(1);

    // act
    MIN_HEAP_ENTRY* result = min_heap_peek(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_036: [ If the heap has no entries, min_heap_peek shall return NULL. ]*/
TEST_FUNCTION(min_heap_peek_after_all_the_entries_are_removed_returns_NULL)
{
    // arrange
    static const uint64_t keys[] = { 10, 20 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));
    min_heap_remove(min_heap, &entries[0]);
    min_heap_remove(min_heap, &entries[1]);

    // act
    MIN_HEAP_ENTRY* result = min_heap_peek(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
    min_heap_destroy(min_heap);
}

/*Tests_SRS_MIN_HEAP_42_037: [ Otherwise, min_heap_peek shall return the entry with the lowest key. ]*/
TEST_FUNCTION(min_heap_peek_returns_the_entry_with_the_lowest_key_without_removing_it)
{
    // arrange
    static const uint64_t keys[] = { 50, 30, 40, 10, 20 };
    static const uint64_t expected_keys[] = { 10, 20, 30, 40, 50 };
    MIN_HEAP_ENTRY entries[MU_COUNT_ARRAY_ITEMS(keys)];

    MIN_HEAP_HANDLE min_heap = test_create_heap_with_reserved_room(TEST_COUNT(keys));
    test_insert_entries(min_heap, entries, keys, TEST_COUNT(keys));

    // act
    MIN_HEAP_ENTRY* result = min_heap_peek(min_heap);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &entries[3], result);
    ASSERT_ARE_EQUAL(uint64_t, 10, result->key);
    ASSERT_ARE_EQUAL(uint32_t, 0, result->index);

    // peeking again returns the same entry, it was not removed
    ASSERT_ARE_EQUAL(void_ptr, &entries[3], min_heap_peek(min_heap));
    test_drain_and_assert_keys(min_heap, entries, TEST_COUNT(entries), expected_keys, TEST_COUNT(expected_keys));

    // cleanup
    min_heap_destroy(min_heap);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
