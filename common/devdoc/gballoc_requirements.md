# gballoc requirements
================

## Overview

gballoc is a module that is a pass through for the malloc, realloc and free memory management functions described in C99, section 7.20.3.
The pass through has the purpose of tracking memory allocations in order to compute the maximal memory usage of an application using the memory management functions.

## References

[ISO/IEC 9899:TC3]

## Exposed API

```c
extern int gballoc_init(void);
extern void gballoc_deinit(void);
extern void* gballoc_malloc(size_t size);
extern void* gballoc_calloc(size_t nmemb, size_t size);
extern void* gballoc_realloc(void* ptr, size_t size);
extern void gballoc_free(void* ptr);
```

### gballoc_init

```c
extern int gballoc_init(void);
```

**SRS_GBALLOC_01_024: [** gballoc_init shall initialize the gballoc module and return 0 upon success. **]**

**SRS_GBALLOC_01_025: [** Init after Init shall fail and return a non-zero value. **]**

**SRS_GBALLOC_01_026: [** gballoc_Init shall create a lock handle that will be used to make the other gballoc APIs thread-safe. **]**

**SRS_GBALLOC_01_027: [** If the Lock creation fails, gballoc_init shall return a non-zero value. **]**

### gballoc_deinit

```c
extern void gballoc_deinit(void);
```

**SRS_GBALLOC_01_028: [** gballoc_deinit shall free all resources allocated by gballoc_init. **]**

**SRS_GBALLOC_01_029: [** if gballoc is not initialized gballoc_deinit shall do nothing. **]**

### gballoc_malloc

```c
extern void* gballoc_malloc(size_t size);
```

**SRS_GBALLOC_01_003: [** gballoc_malloc shall call the C99 malloc function and return its result. **]**

**SRS_GBALLOC_01_012: [** When the underlying malloc call fails, gballoc_malloc shall return NULL. **]**

**SRS_GBALLOC_01_013: [** When gballoc_malloc fails allocating memory for its internal use, gballoc_malloc shall return NULL. **]**

**SRS_GBALLOC_01_030: [** gballoc_malloc shall ensure thread safety by using the lock created by gballoc_Init. **]**

**SRS_GBALLOC_01_039: [** If gballoc was not initialized gballoc_malloc shall simply call malloc without any memory tracking being performed. **]**

**SRS_GBALLOC_01_048: [** If acquiring the lock fails, gballoc_malloc shall return NULL. **]**

### gballoc_calloc

```c
extern void* gballoc_calloc(size_t nmemb, size_t size);
```

**SRS_GBALLOC_01_020: [** gballoc_calloc shall call the C99 calloc function and return its result. **]**

**SRS_GBALLOC_01_022: [** When the underlying calloc call fails, gballoc_calloc shall return NULL. **]**

**SRS_GBALLOC_01_023: [** When gballoc_calloc fails allocating memory for its internal use, gballoc_calloc shall return NULL. **]**

**SRS_GBALLOC_01_031: [** gballoc_calloc shall ensure thread safety by using the lock created by gballoc_Init **]**

**SRS_GBALLOC_01_040: [** If gballoc was not initialized gballoc_calloc shall simply call calloc without any memory tracking being performed. **]**

**SRS_GBALLOC_01_046: [** If acquiring the lock fails, gballoc_calloc shall return NULL. **]**

### gballoc_realloc

```c
extern void* gballoc_realloc(void* ptr, size_t size);
```

**SRS_GBALLOC_01_005: [** gballoc_realloc shall call the C99 realloc function and return its result. **]**

**SRS_GBALLOC_01_014: [** When the underlying realloc call fails, gballoc_realloc shall return NULL. **]**

**SRS_GBALLOC_01_015: [** When allocating memory used for tracking by gballoc_realloc fails, gballoc_realloc shall return NULL. **]**

**SRS_GBALLOC_01_016: [** When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_realloc shall return NULL and the underlying realloc shall not be called. **]**

**SRS_GBALLOC_01_017: [** When ptr is NULL, gballoc_realloc shall call the underlying realloc with ptr being NULL and the realloc result shall be tracked by gballoc. **]**

**SRS_GBALLOC_01_032: [** gballoc_realloc shall ensure thread safety by using the lock created by gballoc_Init. **]**

**SRS_GBALLOC_01_041: [** If gballoc was not initialized gballoc_realloc shall shall simply call realloc without any memory tracking being performed. **]**

**SRS_GBALLOC_01_047: [** If acquiring the lock fails, gballoc_realloc shall return NULL. **]**

### gballoc_free

```c
extern void gballoc_free(void* ptr);
```

**SRS_GBALLOC_01_008: [** gballoc_free shall call the C99 free function. **]**

**SRS_GBALLOC_01_009: [** gballoc_free shall also look up the size associated with the ptr pointer and release the used memory. **]**

**SRS_GBALLOC_01_019: [** When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_free shall not free any memory. **]**

**SRS_GBALLOC_01_033: [** gballoc_free shall ensure thread safety by using the lock created by gballoc_Init. **]**

**SRS_GBALLOC_01_042: [** If gballoc was not initialized gballoc_free shall shall simply call free. **]**

**SRS_GBALLOC_01_049: [** If acquiring the lock fails, gballoc_free shall do nothing. **]**
