# mocked_new_delete requirements

## Overview

`mocked_new_delete` replaces the global C++ `operator new` and `operator delete` (scalar, array and sized) so that unit tests can make C++ allocations fail through a umock-c mock. Tests opt in by linking `c_pal_mocked_new_delete`; linking `c_pal` alone does not replace the operators. Over-aligned allocations are not replaced.

Mocking is enabled from a successful `mocked_new_delete_enable` until `mocked_new_delete_disable`. While it is enabled, `operator new` calls `mocked_new_delete_malloc` while holding an exclusive lock, because umock-c is not thread-safe; otherwise it calls `gballoc_hl_malloc`. `operator delete` always calls `gballoc_hl_free`.

## Usage

- Include `c_pal_test_helpers/mocked_new_delete.h` with mocks enabled and hook `mocked_new_delete_malloc` to `gballoc_hl_malloc` (not `real_gballoc_hl_malloc`), so that `gballoc_hl_free` can release its memory.
- Call `mocked_new_delete_enable` after `umock_c_init` and `mocked_new_delete_disable` before `umock_c_deinit`, while no other thread allocates.
- Hold `mocked_new_delete_lock` while changing expectations that can race with allocations on other threads. Code holding the lock, including the `mocked_new_delete_malloc` hook, must not use C++ `new`, throw, or exit nonlocally.

## Exposed API

```c
MOCKABLE_FUNCTION(, void*, mocked_new_delete_malloc, size_t, size);

int mocked_new_delete_enable(void);

void mocked_new_delete_disable(void);

void mocked_new_delete_lock(void);

void mocked_new_delete_unlock(void);
```

### mocked_new_delete_enable

```c
int mocked_new_delete_enable(void);
```

`mocked_new_delete_enable` routes subsequent `operator new` calls to `mocked_new_delete_malloc`.

**SRS_MOCKED_NEW_DELETE_16_001: [** If mocking is already enabled, `mocked_new_delete_enable` shall return `0` without creating another lock. **]**

**SRS_MOCKED_NEW_DELETE_16_002: [** Otherwise, `mocked_new_delete_enable` shall: **]**

  - **SRS_MOCKED_NEW_DELETE_16_003: [** Create the lock by calling `srw_lock_create` with `false` and an empty lock name. **]**

  - **SRS_MOCKED_NEW_DELETE_16_004: [** If `srw_lock_create` fails, leave mocking disabled and return a non-zero value. **]**

  - **SRS_MOCKED_NEW_DELETE_16_005: [** Otherwise, enable mocking and return `0`. **]**

### mocked_new_delete_disable

```c
void mocked_new_delete_disable(void);
```

`mocked_new_delete_disable` routes subsequent `operator new` calls back to `gballoc_hl_malloc`.

**SRS_MOCKED_NEW_DELETE_16_006: [** If mocking is not enabled, `mocked_new_delete_disable` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_007: [** Otherwise, `mocked_new_delete_disable` shall: **]**

  - **SRS_MOCKED_NEW_DELETE_16_008: [** Disable mocking. **]**

  - **SRS_MOCKED_NEW_DELETE_16_009: [** Destroy the lock by calling `srw_lock_destroy`. **]**

### mocked_new_delete_lock

```c
void mocked_new_delete_lock(void);
```

`mocked_new_delete_lock` blocks mocked allocations on other threads.

**SRS_MOCKED_NEW_DELETE_16_010: [** If mocking is not enabled, `mocked_new_delete_lock` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_011: [** Otherwise, `mocked_new_delete_lock` shall acquire the lock by calling `srw_lock_acquire_exclusive`. **]**

### mocked_new_delete_unlock

```c
void mocked_new_delete_unlock(void);
```

`mocked_new_delete_unlock` releases the lock acquired by `mocked_new_delete_lock`.

**SRS_MOCKED_NEW_DELETE_16_012: [** If mocking is not enabled, `mocked_new_delete_unlock` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_013: [** Otherwise, `mocked_new_delete_unlock` shall release the lock by calling `srw_lock_release_exclusive`. **]**

### operator new

```cpp
void* operator new(size_t size);
```

**SRS_MOCKED_NEW_DELETE_16_014: [** If `size` is `0`, `operator new` shall allocate `1` byte instead. **]**

**SRS_MOCKED_NEW_DELETE_16_015: [** If mocking is not enabled, `operator new` shall allocate memory by calling `gballoc_hl_malloc` with the size to allocate. **]**

**SRS_MOCKED_NEW_DELETE_16_016: [** Otherwise, `operator new` shall: **]**

  - **SRS_MOCKED_NEW_DELETE_16_017: [** Acquire the lock by calling `srw_lock_acquire_exclusive`. **]**

  - **SRS_MOCKED_NEW_DELETE_16_018: [** Allocate memory by calling `mocked_new_delete_malloc` with the size to allocate. **]**

  - **SRS_MOCKED_NEW_DELETE_16_019: [** Release the lock by calling `srw_lock_release_exclusive`. **]**

**SRS_MOCKED_NEW_DELETE_16_020: [** If allocating memory fails, `operator new` shall throw `std::bad_alloc`. **]**

**SRS_MOCKED_NEW_DELETE_16_021: [** Otherwise, `operator new` shall return the allocated memory. **]**

### operator new[]

```cpp
void* operator new[](size_t size);
```

**SRS_MOCKED_NEW_DELETE_16_022: [** `operator new[]` shall return the result of calling `operator new` with `size`, propagating `std::bad_alloc`. **]**

### operator delete

```cpp
void operator delete(void* ptr) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_023: [** If `ptr` is `NULL`, `operator delete` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_024: [** Otherwise, `operator delete` shall free `ptr` by calling `gballoc_hl_free`, even if mocking is enabled. **]**

### operator delete[]

```cpp
void operator delete[](void* ptr) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_025: [** `operator delete[]` shall call `operator delete` with `ptr`. **]**

### sized operator delete

```cpp
void operator delete(void* ptr, size_t size) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_026: [** Sized `operator delete` shall call `operator delete` with `ptr`. **]**

### sized operator delete[]

```cpp
void operator delete[](void* ptr, size_t size) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_027: [** Sized `operator delete[]` shall call `operator delete` with `ptr`. **]**
