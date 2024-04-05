# `MALLOC_MULTI_FLEX` requirements

## Overview

`MALLOC_MULTI_FLEX` allows to allocate memory for a structure containing multiple variable sized array members using a single malloc.

## Exposed API

```c
#define DEFINE_MALLOC_MULTI_FLEX(type, ...)
    ...

#define MALLOC_MULTI_FLEX(type)
    ...
```

## Example - 

Consider there are two structs as defined below -

```c
typedef struct INNER_STURCT_TAG
{
    uint32_t inner_int_1;
    uint64_t inner_int_2;
}INNER_STRUCT;

typedef struct PARENT_STRUCT_TAG
{
    uint64_t int_1;
    uint32_t* array_1;
    uint32_t int_2;
    uint64_t* array_2;
    INNER_STRUCT* array_3;
    uint32_t int_3;
}PARENT_STRUCT;
```

Lets say the user wants to allocate memory for `PARENT_STRUCT` which has three array members:
- array_1 of type `uint32_t`
- array_2 of type `uint64_t`
- array_3 of type `INNER_STRUCT`

The caller first needs to specify the struct and member details using `DEFINE_MALLOC_MULTI_FLEX` macro in `.c` file to create a custom `malloc_multi_flex_<type>` function as defined below -

```c
DEFINE_MALLOC_MULTI_FLEX(PARENT_STRUCT, array_1, array_2, array_3)
```

which will get expanded as following -

```c
static void* malloc_multi_flex_PARENT_STRUCT(size_t parent_struct_size, uint32_t array_1_count, uint32_t array_1_size, uint32_t array_2_count, uint32_t array_2_size, uint32_t array_3_count, uint32_t array_3_size) 
{
    // perform overflow checks while calculating total size to be allocated
    size_t size_tracker = parent_struct_size; 
    if ((array_1_size != 0 && SIZE_MAX / array_1_size < array_1_count) || (SIZE_MAX - size_tracker < array_1_count * array_1_size)) 
    {
        return NULL;
    } 
    size_tracker += array_1_count * array_1_size;
    if ((array_2_size != 0 && SIZE_MAX / array_2_size < array_2_count) || (SIZE_MAX - size_tracker < array_2_count * array_2_size)) 
    {
        return NULL;
    } 
    size_tracker += array_2_count * array_2_size;
    if ((array_3_size != 0 && SIZE_MAX / array_3_size < array_3_count) || (SIZE_MAX - size_tracker < array_3_count * array_3_size)) 
    {
        return NULL;
    } 
    size_tracker += array_3_count * array_3_size;

    // start allocating
    TEST_STRUCT* parent_struct_pointer = gballoc_hl_malloc(size_tracker);
    void* base_size = (char*)parent_struct_pointer + parent_struct_size;
    parent_struct_pointer->array_1 = base_size; 
    base_size = (char*)base_size + array_1_count * array_1_size;
    parent_struct_pointer->array_2 = base_size;
    base_size = (char*)base_size + array_2_count * array_2_size;
    parent_struct_pointer->array_3 = base_size;
    base_size = (char*)base_size + array_3_count * array_3_size; 
    return parent_struct_pointer;
}
```

Now, the user can simply allocate memory for the structure using this statement -

```c
...
uint32_t array_1_count = 10;
uint32_t array_2_count = 20;
uint32_t array_3_count = 30;
PARENT_STRUCT* parent_struct_handle = MALLOC_MULTI_FLEX(PARENT_STRUCT)(sizeof(PARENT_STRUCT), array_1_count, sizeof(uint32_t), array_2_count, sizeof(uint64_t), array_3_count, sizeof(INNER_STRUCT));
// assign and use member arrays
...
```

Note: the order of members specified in the `DEFINE_MALLOC_MULTI_FLEX` should match the array member details provided to `MALLOC_MULTI_FLEX(type) macro`.

### DEFINE_MALLOC_MULTI_FLEX

```c
#define DEFINE_MALLOC_MULTI_FLEX(type, ...)
    ...
```

`DEFINE_MALLOC_MULTI_FLEX` allows defining the memory allocation function for the `type` provided.

**SRS_MALLOC_MULTI_FLEX_24_001: [** If the total amount of memory required to allocate the `type` along with its members exceeds `SIZE_MAX` then `DEFINE_MALLOC_MULTI_FLEX` shall fail and return `NULL`. **]**

**SRS_MALLOC_MULTI_FLEX_24_002: [** `DEFINE_MALLOC_MULTI_FLEX` shall call `malloc` to allocate memory for the struct and its members. **]**

**SRS_MALLOC_MULTI_FLEX_24_003: [** `DEFINE_MALLOC_MULTI_FLEX` shall assign address pointers to all the member arrays. **]**

**SRS_MALLOC_MULTI_FLEX_24_004: [** `DEFINE_MALLOC_MULTI_FLEX` shall succeed and return the address returned by `malloc` function. **]**

### MALLOC_MULTI_FLEX

```c
#define MALLOC_MULTI_FLEX(type) \
    ...
```

**SRS_MALLOC_MULTI_FLEX_24_005: [** `MALLOC_MULTI_FLEX` shall expand `type` to the name of the malloc function in the format of: `MALLOC_MULTI_FLEX_type`. **]**
