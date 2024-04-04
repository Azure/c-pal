# `MALLOC_MULTI_FLEX` requirements

## Overview

`MALLOC_MULTI_FLEX` allows to allocate memory for a structure containing multiple variable sized array members using a single malloc.

## Example - 
Lets say the user wants to allocate memory for `PARENT_STRUCT` which has three array members:
- array_1 of type `uint32_t`
- array_2 of type `uint64_t`
- array_3 of type `INNER_STRUCT`

```
typedef struct INNER_STURCT_TAG
{
    uint32_t inner_single_int;
    uint64_t inner_single_int2;
}INNER_STRUCT;

typedef struct PARENT_STRUCT_TAG
{
    uint64_t single_int;
    uint32_t* array_1;
    uint32_t single_int2;
    uint64_t* array_2;
    INNER_STRUCT* array_3;
    uint32_t single_int3;
}PARENT_STRUCT;
```

The caller first needs to specify the struct and member details using this macro in c file -

```
DEFINE_MALLOC_MULTI_FLEX(PARENT_STRUCT, array_1, array_2, array_3)
```

Then the user can simply allocate memory for the structure using this statement -

```
PARENT_STRUCT* parent_strcut_handle = MALLOC_MULTI_FLEX(PARENT_STRUCT)(sizeof(PARENT_STRUCT), array_1_element_count * sizeof(uint32_t), array_2_element_count * sizeof(uint64_t), array_3_element_count * sizeof(INNER_STRUCT));
```

Note: the order of members specified in the `DEFINE_MALLOC_MULTI_FLEX` should match the array member details provided to `MALLOC_MULTI_FLEX`.

## Exposed API

```c
#define DEFINE_MALLOC_MULTI_FLEX(type, ...)
    ...

#define MALLOC_MULTI_FLEX(type)
    ...
```

### DEFINE_MALLOC_MULTI_FLEX

```c
#define DEFINE_MALLOC_MULTI_FLEX(type, ...)
    ...
```

`DEFINE_MALLOC_MULTI_FLEX` allows defining the memory allocation function for the `type` provided.
