# thandle_ptr_requirements

## Overview

`thandle_ptr` is a module that builds on `THANDLE` and provides a "move existing pointer under THANDLE" functionality.

`THANDLE` has as requirement to "only call malloc once". `THANDLE_PTR` retains that requirement. Note: the existing pointer must have been allocated "somehow" before. Even pointers to stack variables can be moved under `THANDLE_PTR` with the regular gotchas that deallocation happens when leaving the scope of the variable.

`THANDLE_PTR` should be used when the pointer already exists. In all the other situations, where the pointer hasn't been created yet, regular `THANDLE` should be used.

`THANDLE_PTR` works by declaring under the hood a struct as below which captures the parameters passed to `THANDLE_PTR_CREATE_WITH_MOVE` and...

```c
typedef struct PTR_STRUCT_TAG_TYPE_NAME(T)      
{                                               
    T pointer;                                  
    THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) the_free;
} PTR(T);
```    

... and then declaring a regular `THANDLE` over the above defined structure type.

In this document and code implementation "T" is a previously defined type that has pointer semantics and is a C identifier. For example "PS" introduced as: `typedef struct S_TAG* PS;`

## Usage

Usage of `THANDLE_PTR` is very similar to `THANDLE`. 

In a C header, introduce the following declaration:
```c
THANDLE_PTR_DECLARE(A_S_PTR);
```

In a C source file, introduce the following declaration:
```c
THANDLE_PTR_DEFINE(A_S_PTR);
```

Then use it like:
```c
THANDLE(PTR(A_S_PTR)) one = THANDLE_PTR_CREATE_WITH_MOVE(A_S_PTR)(a_s, dispose);
```

`a_s` is a pointer of type `A_S_PTR`.

The above code "moves" ("captures") the pointer `a_s` and produces the `THANDLE` `one`. `dispose` is a user function that takes the original `a_s` pointer and frees all resource used by `a_s`.

`one->pointer` produces the original `a_s` pointer. Other fields of the `one` pointer should not be used (it also captures the `dispose` function for example).

## Exposed API

`THANDLE_PTR` introduces several new types and APIs.

Types are:

`PTR(T)` - is a structure that contains the field `pointer` which captures the original `pointer` passed to `THANDLE_PTR_CREATE_WITH_MOVE`.

`THANDLE(PTR(T))` is the `PTR(T)` above with `THANDLE` semantics.

`THANDLE_PTR_FREE_FUNC_TYPE(T)` is the function pointer type of the `dispose` function. It takes `T` and frees it. `T` is already a pointer.

In addition all `THANDLE` regular APIs apply to `PTR(T)` type. For example
```c
THANDLE_ASSING(PTR(T))(&some_ptr_t, NULL);
```

APIs are:

### THANDLE_PTR_CREATE_WITH_MOVE(T)
```c
THANDLE(PTR(T)) THANDLE_PTR_CREATE_WITH_MOVE(T)(T pointer, THANDLE_PTR_FREE_FUNC_TYPE_NAME(T) dispose );
```


`THANDLE_PTR_CREATE_WITH_MOVE(T)` will "move" `pointer` to a newly created `THANDLE(PTR(T))` which then can be accessed by using the field "pointer" of the structure.


**SRS_THANDLE_PTR_02_001: [** `THANDLE_PTR_CREATE_WITH_MOVE(T)` shall return what `THANDLE_CREATE_FROM_CONTENT(PTR(T))(THANDLE_PTR_DISPOSE(T))` returns. **]**


### THANDLE_PTR_DISPOSE(T)
```
static void THANDLE_PTR_DISPOSE(T)(PTR(T)* ptr)
```

`THANDLE_PTR_DISPOSE` is a static function that calls the original `dispose` passed to `THANDLE_PTR_CREATE_WITH_MOVE(T)`.

**SRS_THANDLE_PTR_02_002: [** If the original `dispose` is non-`NULL` then `THANDLE_PTR_DISPOSE(T)` shall call `dispose`. **]**

**SRS_THANDLE_PTR_02_003: [** `THANDLE_PTR_DISPOSE(T)` shall return. **]**

