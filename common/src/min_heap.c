// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/min_heap.h"

#define MIN_HEAP_INITIAL_CAPACITY 16

typedef struct MIN_HEAP_TAG
{
    MIN_HEAP_ENTRY** entries;
    uint32_t count;
    uint32_t reserved_count;
    uint32_t capacity;
} MIN_HEAP;

static void internal_min_heap_swap_entries(MIN_HEAP* min_heap, uint32_t first, uint32_t second)
{
    MIN_HEAP_ENTRY* temp = min_heap->entries[first];
    min_heap->entries[first] = min_heap->entries[second];
    min_heap->entries[second] = temp;
    min_heap->entries[first]->index = first;
    min_heap->entries[second]->index = second;
}

static void internal_min_heap_sift_up(MIN_HEAP* min_heap, uint32_t index)
{
    while ((index > 0) && (min_heap->entries[index]->key < min_heap->entries[(index - 1) / 2]->key))
    {
        uint32_t parent = (index - 1) / 2;
        internal_min_heap_swap_entries(min_heap, parent, index);
        index = parent;
    }
}

static uint32_t internal_min_heap_get_lowest_of_entry_and_children(MIN_HEAP* min_heap, uint32_t index)
{
    uint32_t result = index;
    uint64_t left = ((uint64_t)index * 2) + 1;
    uint64_t right = left + 1;

    if ((left >= min_heap->count) || (min_heap->entries[left]->key >= min_heap->entries[result]->key))
    {
        /* do nothing */
    }
    else
    {
        result = (uint32_t)left;
    }

    if ((right >= min_heap->count) || (min_heap->entries[right]->key >= min_heap->entries[result]->key))
    {
        /* do nothing */
    }
    else
    {
        result = (uint32_t)right;
    }

    return result;
}

static void internal_min_heap_sift_down(MIN_HEAP* min_heap, uint32_t index)
{
    uint32_t lowest = internal_min_heap_get_lowest_of_entry_and_children(min_heap, index);
    while (lowest != index)
    {
        internal_min_heap_swap_entries(min_heap, lowest, index);
        index = lowest;
        lowest = internal_min_heap_get_lowest_of_entry_and_children(min_heap, index);
    }
}

static int internal_min_heap_ensure_room(MIN_HEAP* min_heap)
{
    int result;

    if (min_heap->reserved_count < min_heap->capacity)
    {
        result = 0;
    }
    else
    {
        /* Codes_SRS_MIN_HEAP_42_041: [ If the reserved count is equal to the capacity of the array of entries: ]*/
        uint32_t new_capacity;

        /* Codes_SRS_MIN_HEAP_42_009: [ min_heap_reserve shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at UINT32_MAX otherwise. ]*/
        if (min_heap->capacity == 0)
        {
            new_capacity = MIN_HEAP_INITIAL_CAPACITY;
        }
        else
        {
            if (min_heap->capacity > UINT32_MAX / 2)
            {
                new_capacity = UINT32_MAX;
            }
            else
            {
                new_capacity = min_heap->capacity * 2;
            }
        }

        /* Codes_SRS_MIN_HEAP_42_010: [ min_heap_reserve shall grow the array of entries to the new capacity by calling realloc_2. ]*/
        MIN_HEAP_ENTRY** new_entries = realloc_2(min_heap->entries, new_capacity, sizeof(MIN_HEAP_ENTRY*));
        if (new_entries == NULL)
        {
            /* Codes_SRS_MIN_HEAP_42_042: [ If realloc_2 fails, min_heap_reserve shall fail and return a non-zero value. ]*/
            LogError("realloc_2(%p, %" PRIu32 ", %zu) failed", min_heap->entries, new_capacity, sizeof(MIN_HEAP_ENTRY*));
            result = MU_FAILURE;
        }
        else
        {
            min_heap->entries = new_entries;
            min_heap->capacity = new_capacity;
            result = 0;
        }
    }

    return result;
}

MIN_HEAP_HANDLE min_heap_create(void)
{
    /* Codes_SRS_MIN_HEAP_42_001: [ min_heap_create shall allocate a new heap by calling malloc. ]*/
    MIN_HEAP* result = malloc(sizeof(MIN_HEAP));
    if (result == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_002: [ If any error occurs, min_heap_create shall fail and return NULL. ]*/
        LogError("malloc(sizeof(MIN_HEAP)=%zu) failed", sizeof(MIN_HEAP));
    }
    else
    {
        /* Codes_SRS_MIN_HEAP_42_038: [ min_heap_create shall set the heap to have no entries, no reserved room and no array of entries. ]*/
        result->entries = NULL;
        result->count = 0;
        result->reserved_count = 0;
        result->capacity = 0;

        /* Codes_SRS_MIN_HEAP_42_039: [ min_heap_create shall succeed and return the heap. ]*/
    }

    return result;
}

void min_heap_destroy(MIN_HEAP_HANDLE min_heap)
{
    if (min_heap == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_003: [ If min_heap is NULL, min_heap_destroy shall return. ]*/
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p", min_heap);
    }
    else
    {
        if (min_heap->entries == NULL)
        {
            /* do nothing */
        }
        else
        {
            /* Codes_SRS_MIN_HEAP_42_004: [ If the heap has an array of entries, min_heap_destroy shall free it. ]*/
            free(min_heap->entries);
        }

        /* Codes_SRS_MIN_HEAP_42_005: [ min_heap_destroy shall free min_heap. ]*/
        free(min_heap);
    }
}

int min_heap_reserve(MIN_HEAP_HANDLE min_heap)
{
    int result;

    if (min_heap == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_006: [ If min_heap is NULL, min_heap_reserve shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p", min_heap);
        result = MU_FAILURE;
    }
    else
    {
        if (min_heap->reserved_count == UINT32_MAX)
        {
            /* Codes_SRS_MIN_HEAP_42_007: [ If the reserved count is UINT32_MAX, min_heap_reserve shall fail and return a non-zero value. ]*/
            LogError("min_heap %p already has room reserved for %" PRIu32 " entries", min_heap, min_heap->reserved_count);
            result = MU_FAILURE;
        }
        else
        {
            if (internal_min_heap_ensure_room(min_heap) != 0)
            {
                /* Codes_SRS_MIN_HEAP_42_042: [ If realloc_2 fails, min_heap_reserve shall fail and return a non-zero value. ]*/
                LogError("internal_min_heap_ensure_room failed");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_MIN_HEAP_42_012: [ min_heap_reserve shall increment the reserved count. ]*/
                min_heap->reserved_count++;

                /* Codes_SRS_MIN_HEAP_42_040: [ min_heap_reserve shall succeed and return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}

void min_heap_unreserve(MIN_HEAP_HANDLE min_heap)
{
    if (min_heap == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_013: [ If min_heap is NULL, min_heap_unreserve shall return. ]*/
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p", min_heap);
    }
    else
    {
        if (min_heap->count == min_heap->reserved_count)
        {
            /* Codes_SRS_MIN_HEAP_42_014: [ If the heap has as many entries as the reserved count, min_heap_unreserve shall return. ]*/
            LogError("min_heap %p has %" PRIu32 " entries and no room reserved for another one", min_heap, min_heap->count);
        }
        else
        {
            /* Codes_SRS_MIN_HEAP_42_015: [ min_heap_unreserve shall decrement the reserved count. ]*/
            min_heap->reserved_count--;
        }
    }
}

void min_heap_entry_init(MIN_HEAP_ENTRY* entry)
{
    if (entry == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_016: [ If entry is NULL, min_heap_entry_init shall return. ]*/
        LogError("Invalid arguments: MIN_HEAP_ENTRY* entry=%p", entry);
    }
    else
    {
        /* Codes_SRS_MIN_HEAP_42_017: [ min_heap_entry_init shall set the key of entry to 0. ]*/
        entry->key = 0;

        /* Codes_SRS_MIN_HEAP_42_018: [ min_heap_entry_init shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
        entry->index = MIN_HEAP_ENTRY_NOT_IN_HEAP;
    }
}

void min_heap_insert(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entry, uint64_t key)
{
    if (
        /* Codes_SRS_MIN_HEAP_42_019: [ If min_heap is NULL, min_heap_insert shall return. ]*/
        (min_heap == NULL) ||
        /* Codes_SRS_MIN_HEAP_42_020: [ If entry is NULL, min_heap_insert shall return. ]*/
        (entry == NULL)
        )
    {
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p, MIN_HEAP_ENTRY* entry=%p, uint64_t key=%" PRIu64 "", min_heap, entry, key);
    }
    else
    {
        if (entry->index != MIN_HEAP_ENTRY_NOT_IN_HEAP)
        {
            /* Codes_SRS_MIN_HEAP_42_021: [ If the index of entry is not MIN_HEAP_ENTRY_NOT_IN_HEAP, min_heap_insert shall return. ]*/
            LogError("entry %p is already in a heap at index %" PRIu32 "", entry, entry->index);
        }
        else
        {
            if (min_heap->count == min_heap->reserved_count)
            {
                /* Codes_SRS_MIN_HEAP_42_022: [ If the heap has as many entries as the reserved count, min_heap_insert shall return. ]*/
                LogError("min_heap %p has %" PRIu32 " entries and no room reserved for another one", min_heap, min_heap->count);
            }
            else
            {
                /* Codes_SRS_MIN_HEAP_42_023: [ min_heap_insert shall set the key of entry to key. ]*/
                entry->key = key;

                /* Codes_SRS_MIN_HEAP_42_024: [ min_heap_insert shall add entry after the last entry of the heap. ]*/
                uint32_t index = min_heap->count;
                min_heap->entries[index] = entry;
                entry->index = index;
                min_heap->count++;

                /* Codes_SRS_MIN_HEAP_42_025: [ min_heap_insert shall swap entry with its parent while its key is lower than the key of its parent. ]*/
                internal_min_heap_sift_up(min_heap, index);
            }
        }
    }
}

void min_heap_remove(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entry)
{
    if (
        /* Codes_SRS_MIN_HEAP_42_026: [ If min_heap is NULL, min_heap_remove shall return. ]*/
        (min_heap == NULL) ||
        /* Codes_SRS_MIN_HEAP_42_027: [ If entry is NULL, min_heap_remove shall return. ]*/
        (entry == NULL)
        )
    {
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p, MIN_HEAP_ENTRY* entry=%p", min_heap, entry);
    }
    else
    {
        if ((entry->index >= min_heap->count) || (min_heap->entries[entry->index] != entry))
        {
            /* Codes_SRS_MIN_HEAP_42_028: [ If entry is not in min_heap, min_heap_remove shall return. ]*/
        }
        else
        {
            uint32_t index = entry->index;

            /* Codes_SRS_MIN_HEAP_42_029: [ min_heap_remove shall remove the last position of the heap. ]*/
            min_heap->count--;

            if (index == min_heap->count)
            {
                // entry was the last entry, nothing moves
            }
            else
            {
                /* Codes_SRS_MIN_HEAP_42_030: [ If entry was not the last entry of the heap: ]*/

                /* Codes_SRS_MIN_HEAP_42_031: [ min_heap_remove shall move the last entry to the position of entry. ]*/
                MIN_HEAP_ENTRY* moved_entry = min_heap->entries[min_heap->count];
                min_heap->entries[index] = moved_entry;
                moved_entry->index = index;

                /* Codes_SRS_MIN_HEAP_42_032: [ min_heap_remove shall swap the moved entry with its child with the lowest key while its key is greater than the key of that child. ]*/
                internal_min_heap_sift_down(min_heap, index);

                /* Codes_SRS_MIN_HEAP_42_033: [ min_heap_remove shall swap the moved entry with its parent while its key is lower than the key of its parent. ]*/
                internal_min_heap_sift_up(min_heap, moved_entry->index);
            }

            /* Codes_SRS_MIN_HEAP_42_034: [ min_heap_remove shall set the index of entry to MIN_HEAP_ENTRY_NOT_IN_HEAP. ]*/
            entry->index = MIN_HEAP_ENTRY_NOT_IN_HEAP;
        }
    }
}

MIN_HEAP_ENTRY* min_heap_peek(MIN_HEAP_HANDLE min_heap)
{
    MIN_HEAP_ENTRY* result;

    if (min_heap == NULL)
    {
        /* Codes_SRS_MIN_HEAP_42_035: [ If min_heap is NULL, min_heap_peek shall return NULL. ]*/
        LogError("Invalid arguments: MIN_HEAP_HANDLE min_heap=%p", min_heap);
        result = NULL;
    }
    else
    {
        if (min_heap->count == 0)
        {
            /* Codes_SRS_MIN_HEAP_42_036: [ If the heap has no entries, min_heap_peek shall return NULL. ]*/
            result = NULL;
        }
        else
        {
            /* Codes_SRS_MIN_HEAP_42_037: [ Otherwise, min_heap_peek shall return the entry with the lowest key. ]*/
            result = min_heap->entries[0];
        }
    }

    return result;
}
