# gballoc design

`gballoc` is a layer that abstracts memory allocators. The need to have and be able to compare multiple memory allocators stems from performance requirements. Not all memory allocators have the same performance and for the sake of being able to compare them gballoc exists.

`gballoc` is divided in 2 layers (`ll` stands for `lower layer`, `hl` stands for `higher layer`):
a) `gballoc_ll` - contains software wrappers over the memory allocation as provided by other components. For example, `gballoc_malloc/free` will redirect to `HeapAlloc/Free` when Windows APIs are directly used.
b) `gballoc_hl` - contains performance measurements for `gballoc_ll` and it is build on top of `gballoc_ll`.


`gballoc_ll` consists of a header file (`gballoc_ll.h`) and several possible implementation of its functions (i.e.: `gballoc_ll_crt.c`, `gballoc_ll_win.c`, `gballoc_ll_mimalloc.c` etc).

`gballoc_hl` consists of a header file (`gballoc_hl.h`) and several possible implementation of the performance functions(i.e. `gballoc_hl_passthrough.c`, `gballoc_hl_buckets.c` etc)

Both `gballoc_ll` and `gballoc_hl` have their own set of orthogonal configurations in CMake. That is - it is possible to use any combination of `gballoc_ll` implementation with any other implementation of `gballoc_ll`. 

As far as Windows is concerned there are several CMake options.

  1. `USE_SEGMENTED_HEAP` - this is a process wide option that replaces all the "regular" heaps with segmented heaps (https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests#heaptype)

  2. Once a heap type is used, there are other several CMakeLists switches that further influence the SW behavior.

    a) `gballoc_ll` implementation is switched between its implementations by the CMake option `GBALLOC_LL_TYPE` (of type string) which can be either

      i. "PASSTHROUGH" - `gballoc_ll` shall route its calls directly to C's routines.

      ii. "WIN32HEAP" - `gballoc_ll` shall route its calls directly to Windows routines 
        (`HeapAlloc`, `HeapReAlloc`, `HeapFree`)

      iii. "MIMALLOC" - `gballoc_ll` shall route its calls directly to `mimalloc` APIs.

    b) `gballoc_hl` implementation is governed by `GBALLOC_HL_TYPE` (of type string) which can be either:

      i. "PASSTHROUGH" - `gballoc_hl` shall route all its APIs directly to `gballoc_ll`.

      ii. "METRICS" - `gballoc_hl` shall construct timing metrics that can be grabbed later.

Linux is not a concern at this moment.

Both `gballoc_ll` and `gballoc_hl` shall have init/deinit functions.
