# interlocked macros

## Overview

`interlocked_macros` contains macros related to interlocked variables.

Currently there is only one such macro, described below.

## Exposed API

```c
#define INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(enum_type, variable_name) \
    union \
    { \
        volatile_atomic int32_t variable_name; \
        enum_type MU_C2(variable_name, _enum); \
    };
```

## INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM

```c
#define INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(enum_type, variable_name)
```

This generates a field like :

```c
    union
    {
        volatile_atomic int32_t variable_name;
        MY_STATE_ENUM variable_name_enum;
    };
```

This is useful for fields which need to be volatile for interlocked operations but are actually an enum (such as a state variable).
The `variable_name_enum` field can be viewed in the debugger to see the value as an enum.

**SRS_INTERLOCKED_MACROS_42_001: [** `INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM` shall generate a union with two fields: a `volatile_atomic` `int32_t` and a variable of the type `enum_type`. **]**
