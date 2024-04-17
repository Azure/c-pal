# `MALLOC_MULTI_FLEX_STRUCT` requirements

## Overview

`MALLOC_MULTI_FLEX_STRUCT` is a set of macros that allow allocating memory for a structure containing multiple variable sized array members using a single malloc.

## Exposed API

```c
#define DECLARE_MALLOC_MULTI_FLEX_STRUCT(type, fields, array_fields)
    ...
    
#define DEFINE_MALLOC_MULTI_FLEX_STRUCT(type, fields, array_fields)
    ...

#define MALLOC_MULTI_FLEX_STRUCT(type)
    ...
```

## Example usage

Lets say the user wants to allocate memory for a structure with name: `PARENT_STRUCT` which has three array members and three non-array members:
- array_1 of type `uint32_t`
- array_2 of type `uint64_t`
- array_3 of type `INNER_STRUCT`
- int_1 of type uint64_t
- int_2 of type uin32_t
- int_4 of type uint32_t

where inner struct is another struct with its own members

```c
typedef struct INNER_STURCT_TAG
{
    uint32_t inner_int_1;
    uint64_t inner_int_2;
}INNER_STRUCT;
```

The caller first needs to specify the struct name, member types and names using `DECLARE_MALLOC_MULTI_FLEX_STRUCT` macro to define the struct and declare the malloc function:

```c
DECLARE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))
```

`DECLARE_MALLOC_MULTI_FLEX_STRUCT` will define the struct as follows:

```c
typedef struct PARENT_STRUCT_TAG
{
    uint64_t int_1;
    uint32_t int_2;
    uint32_t int_3;
    uint32_t* array_1;
    uint64_t* array_2;
    INNER_STRUCT* array_3;
}PARENT_STRUCT;
```

Then, the user should use `DEFINE_MALLOC_MULTI_FLEX_STRUCT` macro to define the custom `malloc_multi_flex_<type>` function -

```c
DEFINE_MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT,
    FIELDS(uint64_t, int_1, uint32_t, int_2, uint32_t, int_3),
    ARRAY_FIELDS(uint32_t, array_1, uint64_t, array_2, INNER_STRUCT, array_3))
```

Now, the user can simply allocate memory for the structure using this statement -

```c
...
uint32_t array_1_count = 10;
uint32_t array_2_count = 20;
uint32_t array_3_count = 30;
PARENT_STRUCT* parent_struct_handle = MALLOC_MULTI_FLEX_STRUCT(PARENT_STRUCT)(sizeof(PARENT_STRUCT), array_1_count, array_2_count, array_3_count);
// assign and use member arrays
...
```

Note: the order of members specified in the `ARRAY_FIELDS` should be in sync with the array members count provided to `MALLOC_MULTI_FLEX_STRUCT(type) macro`.

### DECLARE_MALLOC_MULTI_FLEX_STRUCT

```c
#define DECLARE_MALLOC_MULTI_FLEX_STRUCT(type, fields, array_fields)
    ...
```

`DECLARE_MALLOC_MULTI_FLEX_STRUCT` defines the structure and declares the memory allocation function for the `type` provided.

### DEFINE_MALLOC_MULTI_FLEX_STRUCT

```c
#define DEFINE_MALLOC_MULTI_FLEX_STRUCT(type, fields, array_fields)
    ...
```

`DEFINE_MALLOC_MULTI_FLEX_STRUCT` defines the memory allocation function for the `type` provided.

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_001: [** If the total amount of memory required to allocate the `type` along with its members exceeds `SIZE_MAX` then `DEFINE_MALLOC_MULTI_FLEX_STRUCT` shall fail and return `NULL`. **]**

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_002: [** `DEFINE_MALLOC_MULTI_FLEX_STRUCT` shall call `malloc` to allocate memory for the struct and its members. **]**

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_006: [** If `malloc` fails, `DEFINE_MALLOC_MULTI_FLEX_STRUCT` shall fail and return `NULL`. **]**

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_003: [** `DEFINE_MALLOC_MULTI_FLEX_STRUCT` shall assign address pointers to all the member arrays. **]**

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_004: [** `DEFINE_MALLOC_MULTI_FLEX_STRUCT` shall succeed and return the address returned by `malloc` function. **]**

### FIELDS

```c
#define FIELDS(member_type, member_name ...) \
    ...
```

`FIELDS` macro shall be used with `DECLARE_MALLOC_MULTI_FLEX_STRUCT` or `DEFINE_MALLOC_MULTI_FLEX_STRUCT` to specify the types and names of the non-array members of the struct.

### ARRAY_FIELDS

```c
#define ARRAY_FIELDS(member_type, member_name ...) \
    ...
```

`ARRAY_FIELDS` macro shall be used with `DECLARE_MALLOC_MULTI_FLEX_STRUCT` or `DEFINE_MALLOC_MULTI_FLEX_STRUCT` to specify the types and names of the array members of the struct.

### MALLOC_MULTI_FLEX_STRUCT

```c
#define MALLOC_MULTI_FLEX_STRUCT(type) \
    ...
```

**SRS_MALLOC_MULTI_FLEX_STRUCT_24_005: [** `MALLOC_MULTI_FLEX_STRUCT` shall expand `type` to the name of the malloc function in the format of: `MALLOC_MULTI_FLEX_STRUCT_type`. **]**
