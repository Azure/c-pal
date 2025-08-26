# c-pal AI Coding Instructions

## Project Overview
c-pal is a cross-platform C library that provides a **Platform Abstraction Layer (PAL)** for Azure C libraries. It offers consistent APIs for platform-specific functionality across Windows and Linux, enabling portable code while leveraging platform-optimized implementations underneath.

## Architecture Overview

### Three-Layer Architecture
c-pal is organized into three distinct abstraction layers with clear dependency relationships:

1. **`c_pal_ll/`** (Low-Level PAL) - Core platform abstractions with no external dependencies
   - **Core APIs**: `interlocked.h`, `sync.h`, `threadapi.h` 
   - **No dependencies** beyond standard C library and platform APIs
   - **Platform implementations**: `c_pal_ll/win32/` and `c_pal_ll/linux/`

2. **`common/`** - Utility components that depend only on c_pal_ll
   - **Key utilities**: `refcount.h`, `thandle.h`, `s_list.h`, `sm.h`, `call_once.h`, `lazy_init.h`
   - **Dependencies**: c_pal_ll layer only
   - **Purpose**: Foundational utilities for higher-level components

3. **Top-level PAL** (`interfaces/`, `win32/`, `linux/`) - High-level platform abstractions
   - **Core APIs**: `execution_engine.h`, `threadpool.h`, `async_socket.h`, `file.h`, `pipe.h`
   - **Dependencies**: Both c_pal_ll and common layers
   - **Platform implementations**: `win32/` and `linux/` directories

### Memory Allocator Configuration System
c-pal provides configurable memory allocation through two key variables:

```cmake
# Low-level allocator (controls underlying allocation strategy)
-DGBALLOC_LL_TYPE=PASSTHROUGH|WIN32HEAP|MIMALLOC|JEMALLOC

# High-level allocator (controls metrics and debugging)
-DGBALLOC_HL_TYPE=PASSTHROUGH|METRICS
```

**Critical Build Requirement**: `BUILD_BINARIESDIRECTORY` environment variable must be set to control output paths.

### Core Design Patterns

#### Execution Engine + Async Components Pattern
The execution engine serves as the foundation for all async operations:
```c
// 1. Create execution engine (wraps platform threadpool - PTP_POOL on Windows)
EXECUTION_ENGINE_PARAMETERS params = { .min_thread_count = 4, .max_thread_count = 16 };
EXECUTION_ENGINE_HANDLE engine = execution_engine_create(&params);

// 2. Create async components using the engine
THREADPOOL_HANDLE threadpool = threadpool_create(engine);
ASYNC_SOCKET_HANDLE socket = async_socket_create(engine);
FILE_HANDLE file = file_create(engine);

// 3. All async operations share the same underlying platform threadpool
```

#### Reference Counting with REFCOUNT/THANDLE
Two reference counting systems for different use cases:
```c
// REFCOUNT - Traditional reference counting for C structs
typedef struct MY_OBJECT_TAG { int data; } MY_OBJECT;
DEFINE_REFCOUNT_TYPE(MY_OBJECT);

MY_OBJECT* obj = REFCOUNT_TYPE_CREATE(MY_OBJECT);
INC_REF(MY_OBJECT, obj);  // Increment reference
DEC_REF(MY_OBJECT, obj);  // Decrement, auto-free when reaches 0

// THANDLE - Type-safe smart pointers with move semantics
THANDLE(MY_OBJECT) handle1 = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
THANDLE(MY_OBJECT) handle2 = NULL;
THANDLE_ASSIGN(MY_OBJECT)(&handle2, handle1);     // Shared ownership
THANDLE_MOVE(MY_OBJECT)(&handle2, &handle1);      // Transfer ownership
// Manual cleanup required with THANDLE_ASSIGN(&handle, NULL)
```

#### Platform-Specific Implementation Pattern
Every abstraction follows consistent structure:
```
interfaces/inc/c_pal/component.h          # Cross-platform API
win32/inc/c_pal/component_win32.h         # Windows-specific extensions  
win32/src/component_win32.c               # Windows implementation
linux/inc/c_pal/component_linux.h        # Linux-specific extensions
linux/src/component_linux.c              # Linux implementation
```

## Development Workflows

### Build System Integration
```cmake
# Standard consumption pattern from dependent projects
if ((NOT TARGET c_pal) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-pal/CMakeLists.txt))
    add_subdirectory(deps/c-pal)
endif()

# Link the unified target (automatically selects platform implementation)
target_link_libraries(my_component c_pal)
```

### Environment Setup and Build Commands
```powershell
# Required environment variable for build output
$env:BUILD_BINARIESDIRECTORY = "D:\BUILD_DIR"

# Generate CMake files with allocator selection
cmake -S . -B cmake -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=Debug `
  -DGBALLOC_LL_TYPE=PASSTHROUGH `
  -DGBALLOC_HL_TYPE=PASSTHROUGH

# Build with test options
cmake --build cmake --config Debug
```

### Memory Allocator Testing Strategy
The `tools/build_all.bat` script validates all allocator combinations:
- **GBALLOC_LL_TYPE**: PASSTHROUGH, WIN32HEAP, MIMALLOC
- **GBALLOC_HL_TYPE**: PASSTHROUGH, METRICS  
- **Configurations**: Debug and RelWithDebInfo
- **Total combinations**: 6 LL × 2 HL × 2 configs = 24 test runs

### Test Organization and Execution
```
common/tests/component_ut/       # Unit tests with mocked dependencies
common/tests/component_int/      # Integration tests with real dependencies  
win32/tests/component_ut/        # Platform-specific unit tests
win32/tests/component_int/       # Platform-specific integration tests
```

**Test Categories**:
- `run_unittests=ON` - Component isolation testing with umock-c
- `run_int_tests=ON` - Cross-component and platform integration testing
- `run_perf_tests=ON` - Performance and scalability validation

## Key API Patterns and Usage

### Async Socket Operations (Cross-Platform)
```c
// Create async socket with execution engine
ASYNC_SOCKET_HANDLE socket = async_socket_create(execution_engine);

// Open socket with completion callback
async_socket_open_async(socket, AF_INET, SOCK_STREAM, 0, open_complete_callback, context);

// Send data asynchronously
BUFFER_HANDLE buffers[2] = { buffer1, buffer2 };
async_socket_send_async(socket, buffers, 2, send_complete_callback, context);

// Receive with continuous callback pattern
async_socket_receive_async(socket, receive_callback, context);
```

## THANDLE: Type-Safe Smart Pointers

THANDLE provides C++-like smart pointer functionality for C, offering automatic memory management, type safety, and move semantics. It's a completely separate system from REFCOUNT and uses its own set of macros and functions.

### Core THANDLE Concepts

#### Declaration and Creation
```c
// Declare a THANDLE type for any structure
typedef struct MY_OBJECT_TAG 
{ 
    int data; 
    char* name;
} MY_OBJECT;

// Enable THANDLE support for the type (in header)
THANDLE_TYPE_DECLARE(MY_OBJECT);

// Define THANDLE implementation (in source file)
THANDLE_TYPE_DEFINE(MY_OBJECT);

// Create THANDLE instances using proper THANDLE macros
THANDLE(MY_OBJECT) handle = THANDLE_MALLOC(MY_OBJECT)(dispose_function);  // Creates with ref count 1
THANDLE(MY_OBJECT) null_handle = NULL;                                    // NULL handle

// Create from existing content
MY_OBJECT source = { .data = 42, .name = "test" };
THANDLE(MY_OBJECT) copy_handle = THANDLE_CREATE_FROM_CONTENT(MY_OBJECT)(&source, dispose_function, copy_function);
```

### THANDLE Creation Methods

#### THANDLE_MALLOC - Basic Object Creation
```c
// Simple object creation with dispose function
THANDLE(MY_OBJECT) obj = THANDLE_MALLOC(MY_OBJECT)(dispose_function);

// No dispose function needed (for simple data)
THANDLE(MY_OBJECT) simple_obj = THANDLE_MALLOC(MY_OBJECT)(NULL);
```

#### THANDLE_MALLOC_FLEX - Flexible Array Members
```c
// For structs with flexible array members
typedef struct STRING_OBJECT_TAG
{
    size_t length;
    char data[];  // Flexible array member
} STRING_OBJECT;

// Allocate extra space for the flexible array
THANDLE(STRING_OBJECT) str_obj = THANDLE_MALLOC_FLEX(STRING_OBJECT)(dispose_function, 100, sizeof(char));  // 100 chars
```

#### THANDLE_CREATE_FROM_CONTENT - Copy Construction
```c
// Copy from existing object
MY_OBJECT original = { .data = 42, .name = "original" };
THANDLE(MY_OBJECT) copy = THANDLE_CREATE_FROM_CONTENT(MY_OBJECT)(&original, dispose_function, copy_function);

// Simple copy (memcpy) without custom copy function
THANDLE(MY_OBJECT) simple_copy = THANDLE_CREATE_FROM_CONTENT(MY_OBJECT)(&original, dispose_function, NULL);
```

#### THANDLE_CREATE_FROM_CONTENT_FLEX - Flexible Array Copy
```c
// Copy flexible array structures with custom size calculation
THANDLE(STRING_OBJECT) flex_copy = THANDLE_CREATE_FROM_CONTENT_FLEX(STRING_OBJECT)(
    &original_string, 
    dispose_function, 
    copy_function, 
    get_sizeof_function);  // Custom function to calculate total size
```

#### Assignment and Move Semantics
```c
// THANDLE_ASSIGN - Creates shared ownership (increments ref count)
THANDLE(MY_OBJECT) handle1 = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
THANDLE(MY_OBJECT) handle2;
THANDLE_ASSIGN(MY_OBJECT)(&handle2, handle1);    // Both point to same object, ref count = 2

// THANDLE_MOVE - Transfers ownership (no ref count change)
THANDLE(MY_OBJECT) handle3;
THANDLE_MOVE(MY_OBJECT)(&handle3, &handle1);     // handle3 gets ownership, handle1 becomes NULL

// THANDLE_INITIALIZE - Safe initialization from another THANDLE
THANDLE(MY_OBJECT) handle4;
THANDLE_INITIALIZE(MY_OBJECT)(&handle4, handle2); // Takes shared ownership
```

#### Manual Cleanup and Scope Management
```c
void example_function(void)
{
    THANDLE(MY_OBJECT) local_handle = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
    
    // Use the handle
    local_handle->data = 42;
    
    // Manual cleanup required - THANDLE does NOT have automatic scope cleanup
    THANDLE_ASSIGN(MY_OBJECT)(&local_handle, NULL);  // Decrements ref count, frees if count reaches 0
    
    // Or let caller manage cleanup by not setting to NULL
}
```

### Advanced THANDLE Patterns

#### Function Parameters and Return Values
```c
// Passing THANDLE by value (preferred pattern)
int process_object(THANDLE(MY_OBJECT) object)
{
    if (object == NULL)
    {
        return -1;
    }
    
    // Use object->data directly
    return object->data;
    // No cleanup needed - caller retains ownership
}

// Returning THANDLE (caller gets ownership)
THANDLE(MY_OBJECT) create_object_with_data(int initial_data)
{
    THANDLE(MY_OBJECT) result = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
    if (result != NULL)
    {
        result->data = initial_data;
    }
    return result;  // Caller gets ownership
}

// Output parameter pattern (for optional results)
int try_get_object(THANDLE(MY_OBJECT)* out_object)
{
    THANDLE(MY_OBJECT) temp = create_some_object();
    if (temp == NULL)
    {
        return -1;
    }
    
    THANDLE_MOVE(MY_OBJECT)(out_object, &temp);  // Transfer to caller
    return 0;
}
```

#### Collections and Data Structures
```c
// THANDLE in arrays
typedef struct OBJECT_COLLECTION_TAG
{
    THANDLE(MY_OBJECT)* objects;
    size_t count;
    size_t capacity;
} OBJECT_COLLECTION;

void add_object_to_collection(OBJECT_COLLECTION* collection, THANDLE(MY_OBJECT) object)
{
    // Resize if needed
    if (collection->count >= collection->capacity)
    {
        // ... resize logic ...
    }
    
    // Store shared reference
    THANDLE_ASSIGN(MY_OBJECT)(&collection->objects[collection->count], object);
    collection->count++;
}

void cleanup_collection(OBJECT_COLLECTION* collection)
{
    // Clean up all handles
    for (size_t i = 0; i < collection->count; i++)
    {
        THANDLE_ASSIGN(MY_OBJECT)(&collection->objects[i], NULL);  // Decrements ref count
    }
    free(collection->objects);
    collection->objects = NULL;
    collection->count = 0;
}
```

#### Thread Safety and Shared Objects
```c
// THANDLE provides thread-safe reference counting
// Multiple threads can safely share THANDLE instances

typedef struct SHARED_CONTEXT_TAG
{
    THANDLE(MY_OBJECT) shared_object;
    // ... other fields
} SHARED_CONTEXT;

// Worker thread function
DWORD WINAPI worker_thread(LPVOID param)
{
    SHARED_CONTEXT* context = (SHARED_CONTEXT*)param;
    
    // Create local reference to shared object
    THANDLE(MY_OBJECT) local_ref;
    THANDLE_ASSIGN(MY_OBJECT)(&local_ref, context->shared_object);
    
    // Use object safely - it won't be freed while this thread holds a reference
    if (local_ref != NULL)
    {
        // ... work with local_ref->data ...
    }
    
    // Manual cleanup required before thread exits
    THANDLE_ASSIGN(MY_OBJECT)(&local_ref, NULL);
    return 0;
}
```

### THANDLE vs Raw Pointers vs REFCOUNT

| Feature | Raw Pointer | REFCOUNT | THANDLE |
|---------|-------------|----------|---------|
| **Memory Management** | Manual | Semi-automatic | Automatic |
| **Type Safety** | None | Some | Strong |
| **Null Safety** | Prone to errors | Some protection | Built-in checks |
| **Move Semantics** | N/A | Manual | Built-in |
| **Scope Safety** | Manual cleanup | Manual cleanup | Manual cleanup |
| **Thread Safety** | None | Atomic ref counting | Atomic ref counting |
| **Performance** | Fastest | Fast | Slight overhead |

### Common THANDLE Patterns

#### Factory Pattern with THANDLE
```c
// Factory function returning THANDLE
THANDLE(DATABASE_CONNECTION) database_connection_create(const char* connection_string)
{
    THANDLE(DATABASE_CONNECTION) result = THANDLE_MALLOC(DATABASE_CONNECTION)(dispose_function);
    if (result != NULL)
    {
        if (initialize_connection(result, connection_string) != 0)
        {
            THANDLE_ASSIGN(DATABASE_CONNECTION)(&result, NULL);  // Clean up on failure
        }
    }
    return result;
}

// Usage
THANDLE(DATABASE_CONNECTION) db = database_connection_create("server=localhost");
if (db != NULL)
{
    execute_query(db, "SELECT * FROM users");
    // Manual cleanup required
    THANDLE_ASSIGN(DATABASE_CONNECTION)(&db, NULL);
}
```

#### Resource Pool with THANDLE
```c
typedef struct CONNECTION_POOL_TAG
{
    THANDLE(DATABASE_CONNECTION)* connections;
    size_t pool_size;
    // ... synchronization primitives
} CONNECTION_POOL;

THANDLE(DATABASE_CONNECTION) pool_acquire_connection(CONNECTION_POOL* pool)
{
    THANDLE(DATABASE_CONNECTION) result = NULL;
    
    // Find available connection
    for (size_t i = 0; i < pool->pool_size; i++)
    {
        if (is_connection_available(&pool->connections[i]))
        {
            THANDLE_ASSIGN(DATABASE_CONNECTION)(&result, pool->connections[i]);
            break;
        }
    }
    
    return result;  // Caller gets shared ownership
}
```

### THANDLE Error Handling

#### Safe Initialization Patterns
```c
// Always check THANDLE creation results
THANDLE(MY_OBJECT) safe_create_object(void)
{
    THANDLE(MY_OBJECT) result = NULL;
    
    result = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
    if (result == NULL)
    {
        LogError("Failed to create MY_OBJECT");
    }
    else
    {
        if (initialize_object_data(result) != 0)
        {
            LogError("Failed to initialize object data");
        }
        else
        {
            // all OK
            goto all_ok;
        }

        THANDLE_FREE(MY_OBJECT)(result);
        result = NULL;
    }
    
all_ok:
    return result;
}
```

#### Chain of Operations
```c
// THANDLE enables safe chaining of operations
int process_pipeline(THANDLE(INPUT_DATA) input)
{
    int result;
    THANDLE(PROCESSED_DATA) stage1_result = NULL;
    THANDLE(FINAL_DATA) final_result = NULL;
    
    if (input == NULL)
    {
        result = -1;
        goto cleanup;
    }
    
    stage1_result = process_stage_1(input);
    if (stage1_result == NULL)
    {
        result = -1;
        goto cleanup;
    }
    
    final_result = process_stage_2(stage1_result);
    if (final_result == NULL)
    {
        result = -1;
        goto cleanup;
    }
    
    result = save_results(final_result);
    
cleanup:
    // All THANDLE variables require manual cleanup if desired
    return result;
}
```

### THANDLE Best Practices

#### ✅ Recommended Patterns
```c
// Use THANDLE for all reference-counted objects
THANDLE(MY_OBJECT) obj = create_object();

// Use THANDLE_ASSIGN for sharing
THANDLE_ASSIGN(MY_OBJECT)(&shared_obj, obj);

// Use THANDLE_MOVE for transfer of ownership
THANDLE_MOVE(MY_OBJECT)(&new_owner, &old_owner);

// Always initialize THANDLE variables
THANDLE(MY_OBJECT) handle = NULL;  // Required because THANDLE is const
```

#### ❌ Anti-Patterns to Avoid
```c
// ❌ Don't mix THANDLE with raw pointer operations
THANDLE(MY_OBJECT) handle = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
MY_OBJECT* raw = handle;  // DANGEROUS - loses reference tracking
free(raw);                // WRONG - double-free possible

// ❌ Don't manually manage reference counts with THANDLE
THANDLE(MY_OBJECT) handle = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
THANDLE_INC_REF(MY_OBJECT)(handle);  // WRONG - THANDLE manages this automatically

// ❌ Don't forget to initialize THANDLE variables
THANDLE(MY_OBJECT) handle;   // WRONG - uninitialized
// Should be: THANDLE(MY_OBJECT) handle = NULL;

// ❌ Don't mix THANDLE with REFCOUNT macros
THANDLE(MY_OBJECT) handle = THANDLE_MALLOC(MY_OBJECT)(dispose_function);
INC_REF(MY_OBJECT, handle);  // WRONG - these are REFCOUNT macros, not THANDLE
DEC_REF(MY_OBJECT, handle);  // WRONG - use THANDLE_ASSIGN instead
```

THANDLE provides the safety and convenience of modern smart pointers while maintaining C compatibility and performance. It's the preferred pattern for managing object lifetimes in c-pal components.

### Threadpool Work Scheduling
```c
// Open threadpool for work scheduling
threadpool_open_async(threadpool, open_complete_callback, context);

// Schedule work items
threadpool_schedule_work(threadpool, work_function, work_context);

// Close when done (waits for completion)
threadpool_close(threadpool);
```

### File I/O with Async Operations
```c
// Create and open file
FILE_HANDLE file = file_create(execution_engine, "path/to/file.dat");
file_open_async(file, FILE_GENERIC_READ | FILE_GENERIC_WRITE, 
                FILE_SHARE_READ, FILE_CREATE_ALWAYS, NULL, open_callback, context);

// Async read/write at specific positions
file_read_async(file, buffer, size, position, read_callback, context);
file_write_async(file, buffer, size, position, write_callback, context);
```

### State Machine (SM) for Complex Logic
```c
// Define states and events
DEFINE_ENUM(MY_STATE, IDLE, CONNECTING, CONNECTED, ERROR);
DEFINE_ENUM(MY_EVENT, CONNECT, DATA_RECEIVED, DISCONNECT, ERROR_OCCURRED);

// Create state machine with transition table
SM_HANDLE state_machine = sm_create("my_component");
sm_open_begin(state_machine);
sm_add_transition(state_machine, IDLE, CONNECT, CONNECTING, connect_action);
sm_add_transition(state_machine, CONNECTING, DATA_RECEIVED, CONNECTED, data_action);
sm_open_end(state_machine);

// Execute state machine
sm_exec(state_machine, MY_EVENT_CONNECT);
```

### Safe List Operations (s_list)
```c
// Create list with item cleanup function
S_LIST_HANDLE list = s_list_create(my_item_cleanup_function);

// Add items (automatically managed memory)
S_LIST_ENTRY* entry = s_list_add(list, my_item_data);

// Thread-safe iteration
S_LIST_ENTRY* current = s_list_head(list);
while (current != NULL)
{
    MY_ITEM* item = s_list_entry_get_data(current);
    // Process item
    current = s_list_next(current);
}

s_list_destroy(list);  // Automatically calls cleanup for all items
```

## Platform-Specific Features

### Windows-Specific APIs
```c
#include "c_pal/execution_engine_win32.h"
#include "c_pal/pipe_win32.h" 

// Access underlying Windows handles when needed
PTP_POOL threadpool = execution_engine_win32_get_threadpool(engine);
HANDLE pipe_handle = pipe_win32_get_handle(pipe);

// Windows-specific pipe creation with security attributes
PIPE_HANDLE pipe = pipe_win32_create_with_security(pipe_name, security_attributes);
```

### Linux-Specific APIs  
```c
#include "c_pal/execution_engine_linux.h"
#include "c_pal/socket_handle_linux.h"

// Access Linux-specific features
int epoll_fd = execution_engine_linux_get_epoll(engine);
int socket_fd = socket_handle_linux_get_socket(socket_handle);
```

### Cross-Platform Socket Handling
```c
// Unified socket handle creation (platform-agnostic)
SOCKET_HANDLE socket_handle = socket_handle_create(AF_INET, SOCK_STREAM, 0);

// Platform-specific access when needed
#ifdef WIN32
    SOCKET win_socket = socket_handle_get_socket(socket_handle);  // Returns SOCKET
#else
    int linux_socket = socket_handle_get_socket(socket_handle);   // Returns int
#endif
```

## Testing Patterns and Mocking

### Unit Test Structure with umock-c
```c
#include "testrunnerswitcher.h"
#include "c_pal/interlocked.h"          // Usually not mocked - atomic operations

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"           // Mock memory allocation
#include "c_pal/threadapi.h"            // Mock threading primitives
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"            // Real implementations when needed
#include "c_pal/refcount.h"             // Component under test

// Tests_SRS_EXAMPLE_01_001: [ example_function shall allocate memory for the object. ]
TEST_FUNCTION(example_function_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn((void*)0x1234);

    // act
    MY_OBJECT* result = REFCOUNT_TYPE_CREATE(MY_OBJECT);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}
```

### Integration Test Patterns
```c
// Real platform integration testing
TEST_FUNCTION(execution_engine_threadpool_integration)
{
    // arrange - use real implementations
    EXECUTION_ENGINE_PARAMETERS params = { 2, 8 };
    EXECUTION_ENGINE_HANDLE engine = execution_engine_create(&params);
    THREADPOOL_HANDLE threadpool = threadpool_create(engine);
    
    // act - schedule real work
    bool work_executed = false;
    threadpool_schedule_work(threadpool, test_work_function, &work_executed);
    
    // Wait for work completion
    ThreadAPI_Sleep(100);
    
    // assert
    ASSERT_IS_TRUE(work_executed);
    
    // cleanup
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(engine);
}
```

### Performance Testing Approach
```c
TEST_FUNCTION(async_socket_performance_test)
{
    // arrange
    LARGE_INTEGER start, end, frequency;
    QueryPerformanceFrequency(&frequency);
    
    // act - measure async operation latency
    QueryPerformanceCounter(&start);
    async_socket_send_async(socket, buffers, buffer_count, perf_callback, &completion_event);
    WaitForSingleObject(completion_event, INFINITE);
    QueryPerformanceCounter(&end);
    
    // assert - verify performance threshold
    double latency_ms = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / frequency.QuadPart;
    ASSERT_IS_TRUE(latency_ms < MAX_EXPECTED_LATENCY_MS);
}
```

## Common Patterns and Anti-Patterns

### ✅ Recommended Patterns
```c
// Use execution engine for all async components
EXECUTION_ENGINE_HANDLE engine = execution_engine_create(&params);
THREADPOOL_HANDLE pool = threadpool_create(engine);  // Shared engine
ASYNC_SOCKET_HANDLE sock = async_socket_create(engine);  // Same engine

// Proper THANDLE lifecycle management
THANDLE(MY_OBJECT) handle = create_my_object();
THANDLE(MY_OBJECT) handle2;
THANDLE_ASSIGN(MY_OBJECT)(&handle2, handle);  // Shared reference
// Both handles require manual cleanup when desired

// Use lazy_init for one-time initialization
static LAZY_INIT_HANDLE g_init = LAZY_INIT_STATIC_INIT;
if (lazy_init_async(&g_init, init_function, init_context, &init_result) == LAZY_INIT_OK)
{
    // Initialization guaranteed to happen exactly once
}

// Use call_once for thread-safe singleton initialization
static CALL_ONCE_HANDLE g_once = CALL_ONCE_STATIC_INIT;
call_once(&g_once, initialize_singleton);
```

### ❌ Anti-Patterns to Avoid
```c
// ❌ Don't create multiple execution engines unnecessarily
EXECUTION_ENGINE_HANDLE engine1 = execution_engine_create(&params);
EXECUTION_ENGINE_HANDLE engine2 = execution_engine_create(&params);  // Wasteful
THREADPOOL_HANDLE pool1 = threadpool_create(engine1);
THREADPOOL_HANDLE pool2 = threadpool_create(engine2);

// ✅ Share one execution engine across components
EXECUTION_ENGINE_HANDLE shared_engine = execution_engine_create(&params);
THREADPOOL_HANDLE pool = threadpool_create(shared_engine);
ASYNC_SOCKET_HANDLE socket = async_socket_create(shared_engine);

// ❌ Don't manually manage reference counts with THANDLE
THANDLE(MY_OBJECT) handle = create_object();
// manual_inc_ref(handle);  // WRONG - THANDLE manages this
// manual_dec_ref(handle);  // WRONG - THANDLE manages this

// ❌ Don't mix REFCOUNT and THANDLE patterns
MY_OBJECT* obj = REFCOUNT_TYPE_CREATE(MY_OBJECT);
THANDLE(MY_OBJECT) handle = (THANDLE(MY_OBJECT))obj;  // WRONG - type mismatch

// ❌ Don't forget platform-specific cleanup
#ifdef WIN32
    CloseHandle(platform_handle);  // Required on Windows
#else
    close(platform_handle);        // Required on Linux  
#endif
```

## Debugging and Diagnostics

### Memory Allocator Debugging
```cmake
# Use METRICS allocator to track memory usage
-DGBALLOC_HL_TYPE=METRICS

# Enable detailed allocation logging
-Dlogging_level=VERBOSE
```

### Platform-Specific Debugging
```c
// Windows: Enable ETW tracing for async operations
#ifdef WIN32
    // Use Event Viewer to view c-pal ETW events
    #include "c_logging/log_sink_etw.h"
#endif

// Linux: Use strace to monitor system calls
// strace -e trace=epoll_wait,epoll_ctl ./my_program
```

## External Dependencies & Standards

This project inherits comprehensive build and coding standards from its dependencies:

### Build Infrastructure (`deps/c-build-tools/`)
- **CMake Functions**: `set_default_build_options()`, `use_vcpkg()`, `add_vld_if_defined()`
- **Pipeline Templates**: Azure DevOps CI/CD automation with security scanning
- **Quality Gates**: Traceability validation, SARIF analysis, reals checking

### Core Dependencies
- **`deps/c-logging/`**: Structured logging with ETW support (Windows) and console output
- **`deps/umock-c/`**: C mocking framework for unit testing with type-safe mock generation  
- **`deps/macro-utils-c/`**: Metaprogramming utilities for code generation
- **`deps/ctest/`**: Test execution framework with cross-platform support
- **`deps/c-testrunnerswitcher/`**: Test runner abstraction layer

### Coding Standards Compliance
**CRITICAL**: All code must follow standards in `deps/c-build-tools/.github/general_coding_instructions.md`:
- **Function naming**: snake_case with module prefixes (`c_pal_*`, `threadpool_*`, `async_socket_*`)
- **Requirements traceability**: `SRS_MODULE_##_###` → `Codes_SRS_MODULE_##_###` → `Tests_SRS_MODULE_##_###`
- **Error handling**: Consistent patterns with goto cleanup and single exit points
- **Header inclusion**: Fixed order with `macro_utils.h` first, `c_logging/logger.h` second
- **Memory management**: Always include `c_pal/gballoc_hl_redirect.h` in source files

## Key Files and Entry Points
- **`CMakeLists.txt`**: Root build configuration with allocator options and platform detection
- **`interfaces/inc/c_pal/`**: Cross-platform API definitions for all components
- **`common/inc/c_pal/refcount.h`**: Reference counting macros and patterns
- **`common/inc/c_pal/thandle.h`**: Type-safe smart pointer implementation
- **`doc/pal.md`**: Architecture overview and design rationale
- **`tools/build_all.bat`**: Comprehensive build validation across all allocator combinations

This PAL enables Azure C libraries to write portable code while leveraging platform-optimized implementations, with robust testing across multiple memory allocators and platforms.
