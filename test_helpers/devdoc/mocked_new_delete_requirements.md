# mocked_new_delete requirements

## Overview

mocked_new_delete replaces the global C++ `operator new` and `operator delete` so that unit tests can make C++ allocations fail through a umock-c mock. Tests opt in by linking `c_pal_mocked_new_delete`; linking `c_pal` alone does not replace the operators.

mocked_new_delete starts disabled; `mocked_new_delete_enable` enables it and `mocked_new_delete_disable` disables it. While mocked_new_delete is enabled, `operator new` calls `mocked_new_delete_malloc` while holding an exclusive lock, because umock-c is not thread-safe; otherwise it calls `gballoc_hl_malloc`. `operator delete` always calls `gballoc_hl_free`.

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

**SRS_MOCKED_NEW_DELETE_16_001: [** If mocked_new_delete is already enabled, `mocked_new_delete_enable` shall return `0`. **]**

**SRS_MOCKED_NEW_DELETE_16_002: [** Otherwise: **]**

  - **SRS_MOCKED_NEW_DELETE_16_003: [** `mocked_new_delete_enable` shall create the lock by calling `srw_lock_create` with `false` and an empty lock name. **]**

  - **SRS_MOCKED_NEW_DELETE_16_004: [** `mocked_new_delete_enable` shall mark mocked_new_delete as enabled. **]**

  - **SRS_MOCKED_NEW_DELETE_16_005: [** `mocked_new_delete_enable` shall return `0` on success. **]**

**SRS_MOCKED_NEW_DELETE_16_006: [** If there are any failures, `mocked_new_delete_enable` shall fail and return a non-zero value. **]**

### mocked_new_delete_disable

```c
void mocked_new_delete_disable(void);
```

`mocked_new_delete_disable` routes subsequent `operator new` calls back to `gballoc_hl_malloc`.

**SRS_MOCKED_NEW_DELETE_16_007: [** If mocked_new_delete is disabled, `mocked_new_delete_disable` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_008: [** Otherwise: **]**

  - **SRS_MOCKED_NEW_DELETE_16_009: [** `mocked_new_delete_disable` shall mark mocked_new_delete as disabled. **]**

  - **SRS_MOCKED_NEW_DELETE_16_010: [** `mocked_new_delete_disable` shall destroy the lock by calling `srw_lock_destroy`. **]**

### mocked_new_delete_lock

```c
void mocked_new_delete_lock(void);
```

`mocked_new_delete_lock` blocks mocked allocations on other threads.

**SRS_MOCKED_NEW_DELETE_16_011: [** If mocked_new_delete is disabled, `mocked_new_delete_lock` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_012: [** Otherwise, `mocked_new_delete_lock` shall acquire the lock by calling `srw_lock_acquire_exclusive`. **]**

### mocked_new_delete_unlock

```c
void mocked_new_delete_unlock(void);
```

`mocked_new_delete_unlock` releases the lock acquired by `mocked_new_delete_lock`.

**SRS_MOCKED_NEW_DELETE_16_013: [** If mocked_new_delete is disabled, `mocked_new_delete_unlock` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_014: [** Otherwise, `mocked_new_delete_unlock` shall release the lock by calling `srw_lock_release_exclusive`. **]**

### operator new

```cpp
void* operator new(size_t size);
```

**SRS_MOCKED_NEW_DELETE_16_015: [** If `size` is `0`, `operator new` shall allocate `1` byte instead. **]**

**SRS_MOCKED_NEW_DELETE_16_016: [** If mocked_new_delete is disabled, `operator new` shall allocate memory by calling `gballoc_hl_malloc` with the size to allocate. **]**

**SRS_MOCKED_NEW_DELETE_16_017: [** Otherwise: **]**

  - **SRS_MOCKED_NEW_DELETE_16_018: [** `operator new` shall acquire the lock by calling `srw_lock_acquire_exclusive`. **]**

  - **SRS_MOCKED_NEW_DELETE_16_019: [** `operator new` shall allocate memory by calling `mocked_new_delete_malloc` with the size to allocate. **]**

  - **SRS_MOCKED_NEW_DELETE_16_020: [** `operator new` shall release the lock by calling `srw_lock_release_exclusive`. **]**

**SRS_MOCKED_NEW_DELETE_16_021: [** `operator new` shall return the allocated memory on success. **]**

**SRS_MOCKED_NEW_DELETE_16_022: [** If there are any failures, `operator new` shall throw `std::bad_alloc`. **]**

### operator new[]

```cpp
void* operator new[](size_t size);
```

**SRS_MOCKED_NEW_DELETE_16_023: [** `operator new[]` shall return the result of calling `operator new` with `size`. **]**

### operator delete

```cpp
void operator delete(void* ptr) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_024: [** If `ptr` is `NULL`, `operator delete` shall return. **]**

**SRS_MOCKED_NEW_DELETE_16_025: [** Otherwise, `operator delete` shall free `ptr` by calling `gballoc_hl_free`. **]**

### operator delete[]

```cpp
void operator delete[](void* ptr) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_026: [** `operator delete[]` shall call `operator delete` with `ptr`. **]**

### sized operator delete

```cpp
void operator delete(void* ptr, size_t size) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_027: [** Sized `operator delete` shall call `operator delete` with `ptr`. **]**

### sized operator delete[]

```cpp
void operator delete[](void* ptr, size_t size) noexcept;
```

**SRS_MOCKED_NEW_DELETE_16_028: [** Sized `operator delete[]` shall call `operator delete` with `ptr`. **]**
