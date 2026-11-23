# mocked_new_delete requirements

## Overview

`mocked_new_delete` provides an opt-in, test-only replacement for ordinary C++ `operator new`, `operator new[]`, and their unsized and sized delete counterparts. Link `c_pal_mocked_new_delete` explicitly; linking `c_pal` alone does not replace any operators. Over-aligned allocation is outside this helper's scope.

Allocations use `gballoc_hl_malloc` before mocking is enabled and after it is disabled. While enabled, allocations use the test-provided `mocked_malloc`. Deallocations always use `gballoc_hl_free`, so a test's `mocked_malloc` hook must return memory compatible with that allocator. `mocked_free` is retained for compatibility with the existing test helpers, but is not called.

Initialize umock-c and register the `mocked_malloc` hook before calling `use_mocks_for_new_delete`. Check its return value. Call `use_default_malloc_for_new_delete` before deinitializing umock-c or the allocator. Mode changes require that no other thread is allocating or using the helper's lock.

Mocked allocations are serialized because umock-c is not thread-safe. While mocking is enabled, tests can use `lock_mocked_malloc` and `unlock_mocked_malloc` to protect expectation updates from background allocations. Do not allocate with C++ `new` while holding this non-recursive lock.

## Exposed API

```c
MOCKABLE_FUNCTION(, void*, mocked_malloc, size_t, size);

MOCKABLE_FUNCTION(, void, mocked_free, void*, ptr);

int use_mocks_for_new_delete(void);

void use_default_malloc_for_new_delete(void);

void lock_mocked_malloc(void);

void unlock_mocked_malloc(void);
```

### use_mocks_for_new_delete

`use_mocks_for_new_delete` enables mockable allocation for subsequent ordinary C++ allocations.

**SRS_MOCKED_NEW_DELETE_16_001: [** If mocking is not enabled, `use_mocks_for_new_delete` shall create an exclusive lock by calling `srw_lock_create` with `false` and an empty lock name. **]**

**SRS_MOCKED_NEW_DELETE_16_002: [** If `srw_lock_create` fails, `use_mocks_for_new_delete` shall leave mocking disabled and return a non-zero value. **]**

**SRS_MOCKED_NEW_DELETE_16_003: [** `use_mocks_for_new_delete` shall enable mocking and return `0` on success. **]**

**SRS_MOCKED_NEW_DELETE_16_004: [** If mocking is already enabled, `use_mocks_for_new_delete` shall leave the existing lock unchanged and return `0`. **]**

### use_default_malloc_for_new_delete

`use_default_malloc_for_new_delete` disables mockable allocation.

**SRS_MOCKED_NEW_DELETE_16_005: [** If mocking is enabled, `use_default_malloc_for_new_delete` shall disable mocking before destroying the exclusive lock by calling `srw_lock_destroy`. **]**

**SRS_MOCKED_NEW_DELETE_16_006: [** If mocking is not enabled, `use_default_malloc_for_new_delete` shall do nothing. **]**

### lock_mocked_malloc

`lock_mocked_malloc` protects mock expectations while mocking is enabled.

**SRS_MOCKED_NEW_DELETE_16_007: [** If mocking is enabled, `lock_mocked_malloc` shall acquire the exclusive lock by calling `srw_lock_acquire_exclusive`. **]**

**SRS_MOCKED_NEW_DELETE_16_008: [** If mocking is not enabled, `lock_mocked_malloc` shall do nothing. **]**

### unlock_mocked_malloc

`unlock_mocked_malloc` releases the lock acquired by `lock_mocked_malloc`.

**SRS_MOCKED_NEW_DELETE_16_009: [** If mocking is enabled, `unlock_mocked_malloc` shall release the exclusive lock by calling `srw_lock_release_exclusive`. **]**

**SRS_MOCKED_NEW_DELETE_16_010: [** If mocking is not enabled, `unlock_mocked_malloc` shall do nothing. **]**

### operator new

The replacement `operator new(size_t size)` allocates ordinary C++ objects.

**SRS_MOCKED_NEW_DELETE_16_011: [** If `size` is `0`, `operator new` shall request `1` byte instead. **]**

**SRS_MOCKED_NEW_DELETE_16_012: [** If mocking is disabled, `operator new` shall allocate memory by calling `gballoc_hl_malloc`. **]**

**SRS_MOCKED_NEW_DELETE_16_013: [** If mocking is enabled, `operator new` shall acquire the exclusive lock, call `mocked_malloc` with the requested size, and release the exclusive lock. **]**

**SRS_MOCKED_NEW_DELETE_16_014: [** If allocation fails, `operator new` shall throw `std::bad_alloc`. **]**

**SRS_MOCKED_NEW_DELETE_16_015: [** `operator new` shall return the allocated memory on success. **]**

### operator new[]

The replacement `operator new[](size_t size)` allocates ordinary C++ arrays.

**SRS_MOCKED_NEW_DELETE_16_016: [** `operator new[]` shall allocate memory by calling `operator new` with `size`, propagating its result or exception. **]**

### operator delete

**SRS_MOCKED_NEW_DELETE_16_017: [** If `ptr` is `NULL`, `operator delete(void* ptr)` shall do nothing. **]**

**SRS_MOCKED_NEW_DELETE_16_018: [** If `ptr` is not `NULL`, `operator delete(void* ptr)` shall release `ptr` by calling `gballoc_hl_free`, regardless of whether mocking is enabled. **]**

### operator delete[]

**SRS_MOCKED_NEW_DELETE_16_019: [** `operator delete[](void* ptr)` shall release `ptr` by calling `operator delete(void* ptr)`. **]**

### Sized operator delete

**SRS_MOCKED_NEW_DELETE_16_020: [** `operator delete(void* ptr, size_t size)` shall release `ptr` by calling `operator delete(void* ptr)`. **]**

### Sized operator delete[]

**SRS_MOCKED_NEW_DELETE_16_021: [** `operator delete[](void* ptr, size_t size)` shall release `ptr` by calling `operator delete(void* ptr)`. **]**
