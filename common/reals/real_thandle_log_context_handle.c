// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "real_thandle_log_context_handle_renames.h" // IWYU pragma: keep

#include "../src/thandle_log_context_handle.c"

#if 0
typedef void (*THANDLE_PTR_FREE_FUNC_real_LOG_CONTEXT_HANDLE)(real_LOG_CONTEXT_HANDLE arg);
;
typedef struct PTR_STRUCT_real_LOG_CONTEXT_HANDLE_TAG
{
    real_LOG_CONTEXT_HANDLE pointer;
    THANDLE_PTR_FREE_FUNC_real_LOG_CONTEXT_HANDLE dispose;
} PTR_STRUCT_real_LOG_CONTEXT_HANDLE;
;
typedef const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *const volatile CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE;
;
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_ASSIGN(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                               CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t2);
;
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INITIALIZE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                                   CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t2);
;
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_MOVE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                             CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t2);
;
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INITIALIZE_MOVE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                                        CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t2);
;
;
CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE THANDLE_PTR_CREATE_WITH_MOVE_real_LOG_CONTEXT_HANDLE(
    real_LOG_CONTEXT_HANDLE pointer, THANDLE_PTR_FREE_FUNC_real_LOG_CONTEXT_HANDLE dispose);
;
;

typedef struct THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_real_LOG_CONTEXT_HANDLE_TAG
{
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T thandle_ll_malloc;
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T thandle_ll_malloc_flex;
    THANDLE_LL_FREE_FUNCTION_POINTER_T thandle_ll_free;
} THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_real_LOG_CONTEXT_HANDLE;
static const THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_real_LOG_CONTEXT_HANDLE
    THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE = {((void *)0), ((void *)0), ((void *)0)};
typedef struct PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER_TAG
{
    char name[32];
    volatile int32_t refCount;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function;
    void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *);
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE data;
} PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER;
;
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
    void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *), THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function,
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE *result;
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function_used;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;
    if (malloc_function != ((void *)0))
    {
        malloc_function_used = malloc_function;
        free_function_used = free_function;
    }
    else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc != ((void *)0))
    {
        malloc_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc;
        free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_free;
    }
    else
    {
        malloc_function_used = gballoc_hl_malloc;
        free_function_used = gballoc_hl_free;
    }
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
        (PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)malloc_function_used(
            sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER));
    if (handle_impl == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("error in malloc_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   "))=%zu)",
                                   malloc_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER)));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "error in malloc_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       "))=%zu)",
                       malloc_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER));
        } while (0);
        result = ((void *)0);
    }
    else
    {
        (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_real_LOG_CONTEXT_HANDLE");
        ;
        handle_impl->dispose = dispose;
        handle_impl->free_function = free_function_used;
        (void)interlocked_exchange(&handle_impl->refCount, 1);
        result = &(handle_impl->data);
    }
    return result;
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *THANDLE_MALLOC_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
    void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *))
{
    return THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(dispose, ((void *)0),
                                                                                      ((void *)0));
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *
THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
    void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *), size_t nmemb, size_t size,
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE *result;
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function_used;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;
    if (malloc_flex_function != ((void *)0))
    {
        malloc_flex_function_used = malloc_flex_function;
        free_function_used = free_function;
    }
    else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc_flex != ((void *)0))
    {
        malloc_flex_function_used =
            THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc_flex;
        free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_free;
    }
    else
    {
        malloc_flex_function_used = gballoc_hl_malloc_flex;
        free_function_used = gballoc_hl_free;
    }
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
        malloc_flex_function_used(sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER), nmemb, size);
    if (handle_impl == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   "))=%zu)",
                                   malloc_flex_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER)));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       "))=%zu)",
                       malloc_flex_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER));
        } while (0);
        result = ((void *)0);
    }
    else
    {
        (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_real_LOG_CONTEXT_HANDLE");
        ;
        handle_impl->dispose = dispose;
        handle_impl->free_function = free_function_used;
        (void)interlocked_exchange(&handle_impl->refCount, 1);
        result = &(handle_impl->data);
    }
    return result;
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *PTR_STRUCT_real_LOG_CONTEXT_HANDLE_MALLOC_WITH_EXTRA_SIZE(
    void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *), size_t nmemb, size_t size)
{
    return THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
        dispose, nmemb, size, ((void *)0), ((void *)0));
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *
THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
    const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source, void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *),
    int (*copy)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *destination, const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source),
    size_t (*get_sizeof)(const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source),
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE *result;
    if ((source == ((void *)0)) || (get_sizeof == ((void *)0)))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid arguments const T* source=%p, void(*dispose)(T*)=%p, int(*copy)(T* "
                                   "destination, const T* source)=%p, size_t(*get_sizeof)(const T* source)=%p, "
                                   "THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function=%p, "
                                   "THANDLE_LL_FREE_FUNCTION_POINTER_T free_function=%p",
                                   source, dispose, copy, get_sizeof, malloc_flex_function, free_function));
            } while (0);
            logger_log(
                LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                __FUNCTION__, 17,
                "invalid arguments const T* source=%p, void(*dispose)(T*)=%p, int(*copy)(T* destination, const T* "
                "source)=%p, size_t(*get_sizeof)(const T* source)=%p, THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T "
                "malloc_flex_function=%p, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function=%p",
                source, dispose, copy, get_sizeof, malloc_flex_function, free_function);
        } while (0);
        result = ((void *)0);
    }
    else
    {
        THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function_used;
        THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;
        if (malloc_flex_function != ((void *)0))
        {
            malloc_flex_function_used = malloc_flex_function;
            free_function_used = free_function;
        }
        else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc_flex != ((void *)0))
        {
            malloc_flex_function_used =
                THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_malloc_flex;
            free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_real_LOG_CONTEXT_HANDLE.thandle_ll_free;
        }
        else
        {
            malloc_flex_function_used = gballoc_hl_malloc_flex;
            free_function_used = gballoc_hl_free;
        }
        size_t sizeof_source = get_sizeof(source);
        PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
            (PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)malloc_flex_function_used(
                sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER) - sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE), 1,
                sizeof_source);
        if (handle_impl == ((void *)0))
        {
            do
            {
                do
                {
                    (void)(0 && printf("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(T))=%zu "
                                       "- sizeof(T)=%zu, 1, sizeof_source=%zu)",
                                       malloc_flex_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER),
                                       sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE), sizeof_source));
                } while (0);
                logger_log(LOG_LEVEL_ERROR, ((void *)0),
                           "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c", __FUNCTION__, 17,
                           "error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(T))=%zu - "
                           "sizeof(T)=%zu, 1, sizeof_source=%zu)",
                           malloc_flex_function_used, sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER),
                           sizeof(PTR_STRUCT_real_LOG_CONTEXT_HANDLE), sizeof_source);
            } while (0);
            result = ((void *)0);
        }
        else
        {
            if (copy == ((void *)0))
            {
                (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_real_LOG_CONTEXT_HANDLE");
                ;
                (void)memcpy(&(handle_impl->data), source, sizeof_source);
                handle_impl->dispose = dispose;
                handle_impl->free_function = free_function_used;
                (void)interlocked_exchange(&handle_impl->refCount, 1);
                result = &(handle_impl->data);
            }
            else
            {
                if (copy(&handle_impl->data, source) != 0)
                {
                    do
                    {
                        do
                        {
                            (void)(0 && printf("failure in copy(&handle_impl->data=%p, source=%p)", &handle_impl->data,
                                               source));
                        } while (0);
                        logger_log(LOG_LEVEL_ERROR, ((void *)0),
                                   "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c", __FUNCTION__, 17,
                                   "failure in copy(&handle_impl->data=%p, source=%p)", &handle_impl->data, source);
                    } while (0);
                    free_function_used(handle_impl);
                    result = ((void *)0);
                }
                else
                {
                    handle_impl->dispose = dispose;
                    handle_impl->free_function = free_function_used;
                    (void)interlocked_exchange(&handle_impl->refCount, 1);
                    result = &(handle_impl->data);
                }
            }
        }
    }
    return result;
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *PTR_STRUCT_real_LOG_CONTEXT_HANDLE_CREATE_FROM_CONTENT_FLEX(
    const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source, void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *),
    int (*copy)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *destination, const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source),
    size_t (*get_sizeof)(const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source))
{
    return THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_real_LOG_CONTEXT_HANDLE(
        source, dispose, copy, get_sizeof, ((void *)0), ((void *)0));
}
static size_t PTR_STRUCT_real_LOG_CONTEXT_HANDLE_GET_SIZE_OF(const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t)
{
    return sizeof(*t);
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *PTR_STRUCT_real_LOG_CONTEXT_HANDLE_CREATE_FROM_CONTENT(
    const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source, void (*dispose)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *),
    int (*copy)(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *destination, const PTR_STRUCT_real_LOG_CONTEXT_HANDLE *source))
{
    return PTR_STRUCT_real_LOG_CONTEXT_HANDLE_CREATE_FROM_CONTENT_FLEX(source, dispose, copy,
                                                                       PTR_STRUCT_real_LOG_CONTEXT_HANDLE_GET_SIZE_OF);
}
static void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_FREE(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t)
{
    if (t == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid arg "
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   "* t=%p",
                                   t));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "invalid arg "
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       "* t=%p",
                       t);
        } while (0);
    }
    else
    {
        PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
            ((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER
                  *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)0)->data)));
        handle_impl->free_function(handle_impl);
    }
}
static void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_DEC_REF(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
        ((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER
              *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)0)->data)));
    if (interlocked_decrement(&handle_impl->refCount) == 0)
    {
        if (handle_impl->dispose != ((void *)0))
        {
            handle_impl->dispose(&handle_impl->data);
        }
        handle_impl->free_function(handle_impl);
    }
}
static void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INC_REF(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *handle_impl =
        ((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER
              *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)0)->data)));
    (void)interlocked_increment(&handle_impl->refCount);
}
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_ASSIGN(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                               CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t2)
{
    if (t1 == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") t2=%p",
                       t1, t2);
        } while (0);
    }
    else
    {
        if (*t1 == ((void *)0))
        {
            if (t2 == ((void *)0))
            {
            }
            else
            {
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INC_REF(t2);
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = t2;
            }
        }
        else
        {
            if (t2 == ((void *)0))
            {
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_DEC_REF(*t1);
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = t2;
            }
            else
            {
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INC_REF(t2);
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_DEC_REF(*t1);
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = t2;
            }
        }
    }
}
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INITIALIZE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *lvalue,
                                                   CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE rvalue)
{
    if (lvalue == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") * lvalue=%p, THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") rvalue=%p",
                                   lvalue, rvalue));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") * lvalue=%p, THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") rvalue=%p",
                       lvalue, rvalue);
        } while (0);
    }
    else
    {
        if (rvalue == ((void *)0))
        {
        }
        else
        {
            PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INC_REF(rvalue);
        }
        *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)lvalue = rvalue;
    }
}
static PTR_STRUCT_real_LOG_CONTEXT_HANDLE *PTR_STRUCT_real_LOG_CONTEXT_HANDLE_GET_T(
    CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t)
{
    return (PTR_STRUCT_real_LOG_CONTEXT_HANDLE *)t;
}
const PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *const PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INSPECT(
    CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE t)
{
    return ((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER
                 *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_real_LOG_CONTEXT_HANDLE_WRAPPER *)0)->data)));
}
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_MOVE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                             CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t2)
{
    if ((t1 == ((void *)0)) || (t2 == ((void *)0)))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") t2=%p",
                       t1, t2);
        } while (0);
    }
    else
    {
        if (*t1 == ((void *)0))
        {
            if (*t2 == ((void *)0))
            {
            }
            else
            {
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = *t2;
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t2 = ((void *)0);
            }
        }
        else
        {
            if (*t2 == ((void *)0))
            {
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_DEC_REF(*t1);
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = ((void *)0);
            }
            else
            {
                PTR_STRUCT_real_LOG_CONTEXT_HANDLE_DEC_REF(*t1);
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = *t2;
                *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t2 = ((void *)0);
            }
        }
    }
}
void PTR_STRUCT_real_LOG_CONTEXT_HANDLE_INITIALIZE_MOVE(CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t1,
                                                        CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE *t2)
{
    if ((t1 == ((void *)0)) || (t2 == ((void *)0)))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\reals\\../src/thandle_log_context_handle.c",
                       __FUNCTION__, 17,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_real_LOG_CONTEXT_HANDLE"
                       ") t2=%p",
                       t1, t2);
        } while (0);
    }
    else
    {
        if (*t2 == ((void *)0))
        {
            *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = ((void *)0);
        }
        else
        {
            *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t1 = *t2;
            *(PTR_STRUCT_real_LOG_CONTEXT_HANDLE const **)t2 = ((void *)0);
        }
    }
};
static void THANDLE_PTR_DISPOSE_real_LOG_CONTEXT_HANDLE(PTR_STRUCT_real_LOG_CONTEXT_HANDLE *ptr)
{
    if (ptr->dispose != ((void *)0))
    {
        ptr->dispose(ptr->pointer);
    }
    else
    {
    }
}
CONST_P2_CONST_PTR_STRUCT_real_LOG_CONTEXT_HANDLE THANDLE_PTR_CREATE_WITH_MOVE_real_LOG_CONTEXT_HANDLE(
    real_LOG_CONTEXT_HANDLE pointer, THANDLE_PTR_FREE_FUNC_real_LOG_CONTEXT_HANDLE dispose)
{
    PTR_STRUCT_real_LOG_CONTEXT_HANDLE temp = {.pointer = pointer, .dispose = dispose};
    return PTR_STRUCT_real_LOG_CONTEXT_HANDLE_CREATE_FROM_CONTENT(&temp, THANDLE_PTR_DISPOSE_real_LOG_CONTEXT_HANDLE,
                                                                  ((void *)0));
};

#endif
