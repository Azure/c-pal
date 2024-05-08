// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/string_utils.h"

#include "thandle_ptr_user.h"

typedef struct A_S_TAG
{
    int a;
    char* s;
}A_S;

typedef A_S* A_S_PTR;



/*QWQWQW*/
#if 1
THANDLE_PTR_DECLARE(A_S_PTR);
/*WEWEWE*/
THANDLE_PTR_DEFINE(A_S_PTR);
/*ERERER*/
#else
typedef struct PTR_STRUCT_A_S_PTR_TAG
{
    A_S_PTR pointer;
} PTR_STRUCT_A_S_PTR;
;
typedef const PTR_STRUCT_A_S_PTR *const volatile CONST_P2_CONST_PTR_STRUCT_A_S_PTR;
;
void PTR_STRUCT_A_S_PTR_ASSIGN(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR t2);
;
void PTR_STRUCT_A_S_PTR_INITIALIZE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR t2);
;
void PTR_STRUCT_A_S_PTR_MOVE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t2);
;
void PTR_STRUCT_A_S_PTR_INITIALIZE_MOVE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t2);
;
;
;

typedef struct THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_A_S_PTR_TAG
{
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T thandle_ll_malloc;
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T thandle_ll_malloc_flex;
    THANDLE_LL_FREE_FUNCTION_POINTER_T thandle_ll_free;
} THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_A_S_PTR;
static const THANDLE_LL_TYPE_STRUCT_TYPE_PTR_STRUCT_A_S_PTR THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR = {
    ((void *)0), ((void *)0), ((void *)0)};
typedef struct PTR_STRUCT_A_S_PTR_WRAPPER_TAG
{
    char name[32];
    volatile int32_t refCount;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function;
    void (*dispose)(PTR_STRUCT_A_S_PTR *);
    PTR_STRUCT_A_S_PTR data;
} PTR_STRUCT_A_S_PTR_WRAPPER;
;
static PTR_STRUCT_A_S_PTR *THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(
    void (*dispose)(PTR_STRUCT_A_S_PTR *), THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function,
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_A_S_PTR *result;
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function_used;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;
    if (malloc_function != ((void *)0))
    {
        malloc_function_used = malloc_function;
        free_function_used = free_function;
    }
    else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc != ((void *)0))
    {
        malloc_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc;
        free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_free;
    }
    else
    {
        malloc_function_used = gballoc_hl_malloc;
        free_function_used = gballoc_hl_free;
    }
    PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl =
        (PTR_STRUCT_A_S_PTR_WRAPPER *)malloc_function_used(sizeof(PTR_STRUCT_A_S_PTR_WRAPPER));
    if (handle_impl == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("error in malloc_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                                   "PTR_STRUCT_A_S_PTR"
                                   "))=%zu)",
                                   malloc_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER)));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "error in malloc_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                       "PTR_STRUCT_A_S_PTR"
                       "))=%zu)",
                       malloc_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER));
        } while (0);
        result = ((void *)0);
    }
    else
    {
        (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_A_S_PTR");
        ;
        handle_impl->dispose = dispose;
        handle_impl->free_function = free_function_used;
        (void)interlocked_exchange(&handle_impl->refCount, 1);
        result = &(handle_impl->data);
    }
    return result;
}
static PTR_STRUCT_A_S_PTR *THANDLE_MALLOC_PTR_STRUCT_A_S_PTR(void (*dispose)(PTR_STRUCT_A_S_PTR *))
{
    return THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(dispose, ((void *)0), ((void *)0));
}
static PTR_STRUCT_A_S_PTR *THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(
    void (*dispose)(PTR_STRUCT_A_S_PTR *), size_t nmemb, size_t size,
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_A_S_PTR *result;
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function_used;
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;
    if (malloc_flex_function != ((void *)0))
    {
        malloc_flex_function_used = malloc_flex_function;
        free_function_used = free_function;
    }
    else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc_flex != ((void *)0))
    {
        malloc_flex_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc_flex;
        free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_free;
    }
    else
    {
        malloc_flex_function_used = gballoc_hl_malloc_flex;
        free_function_used = gballoc_hl_free;
    }
    PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl =
        malloc_flex_function_used(sizeof(PTR_STRUCT_A_S_PTR_WRAPPER), nmemb, size);
    if (handle_impl == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                                   "PTR_STRUCT_A_S_PTR"
                                   "))=%zu)",
                                   malloc_flex_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER)));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME("
                       "PTR_STRUCT_A_S_PTR"
                       "))=%zu)",
                       malloc_flex_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER));
        } while (0);
        result = ((void *)0);
    }
    else
    {
        (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_A_S_PTR");
        ;
        handle_impl->dispose = dispose;
        handle_impl->free_function = free_function_used;
        (void)interlocked_exchange(&handle_impl->refCount, 1);
        result = &(handle_impl->data);
    }
    return result;
}
static PTR_STRUCT_A_S_PTR *PTR_STRUCT_A_S_PTR_MALLOC_WITH_EXTRA_SIZE(void (*dispose)(PTR_STRUCT_A_S_PTR *),
                                                                     size_t nmemb, size_t size)
{
    return THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(dispose, nmemb, size,
                                                                                            ((void *)0), ((void *)0));
}
static PTR_STRUCT_A_S_PTR *THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(
    const PTR_STRUCT_A_S_PTR *source, void (*dispose)(PTR_STRUCT_A_S_PTR *),
    int (*copy)(PTR_STRUCT_A_S_PTR *destination, const PTR_STRUCT_A_S_PTR *source),
    size_t (*get_sizeof)(const PTR_STRUCT_A_S_PTR *source),
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)
{
    PTR_STRUCT_A_S_PTR *result;
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
                LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c", __FUNCTION__,
                32,
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
        else if (THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc_flex != ((void *)0))
        {
            malloc_flex_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_malloc_flex;
            free_function_used = THANDLE_LL_TYPE_FROM_MACRO_PTR_STRUCT_A_S_PTR.thandle_ll_free;
        }
        else
        {
            malloc_flex_function_used = gballoc_hl_malloc_flex;
            free_function_used = gballoc_hl_free;
        }
        size_t sizeof_source = get_sizeof(source);
        PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl = (PTR_STRUCT_A_S_PTR_WRAPPER *)malloc_flex_function_used(
            sizeof(PTR_STRUCT_A_S_PTR_WRAPPER) - sizeof(PTR_STRUCT_A_S_PTR), 1, sizeof_source);
        if (handle_impl == ((void *)0))
        {
            do
            {
                do
                {
                    (void)(0 && printf("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(T))=%zu "
                                       "- sizeof(T)=%zu, 1, sizeof_source=%zu)",
                                       malloc_flex_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER),
                                       sizeof(PTR_STRUCT_A_S_PTR), sizeof_source));
                } while (0);
                logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                           __FUNCTION__, 32,
                           "error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(T))=%zu - "
                           "sizeof(T)=%zu, 1, sizeof_source=%zu)",
                           malloc_flex_function_used, sizeof(PTR_STRUCT_A_S_PTR_WRAPPER), sizeof(PTR_STRUCT_A_S_PTR),
                           sizeof_source);
            } while (0);
            result = ((void *)0);
        }
        else
        {
            if (copy == ((void *)0))
            {
                (void)snprintf(handle_impl->name, 32, "%s", "PTR_STRUCT_A_S_PTR");
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
                                   "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c", __FUNCTION__, 32,
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
static PTR_STRUCT_A_S_PTR *PTR_STRUCT_A_S_PTR_CREATE_FROM_CONTENT_FLEX(
    const PTR_STRUCT_A_S_PTR *source, void (*dispose)(PTR_STRUCT_A_S_PTR *),
    int (*copy)(PTR_STRUCT_A_S_PTR *destination, const PTR_STRUCT_A_S_PTR *source),
    size_t (*get_sizeof)(const PTR_STRUCT_A_S_PTR *source))
{
    return THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_PTR_STRUCT_A_S_PTR(source, dispose, copy, get_sizeof,
                                                                                     ((void *)0), ((void *)0));
}
static size_t PTR_STRUCT_A_S_PTR_GET_SIZE_OF(const PTR_STRUCT_A_S_PTR *t)
{
    return sizeof(*t);
}
static PTR_STRUCT_A_S_PTR *PTR_STRUCT_A_S_PTR_CREATE_FROM_CONTENT(const PTR_STRUCT_A_S_PTR *source,
                                                                  void (*dispose)(PTR_STRUCT_A_S_PTR *),
                                                                  int (*copy)(PTR_STRUCT_A_S_PTR *destination,
                                                                              const PTR_STRUCT_A_S_PTR *source))
{
    return PTR_STRUCT_A_S_PTR_CREATE_FROM_CONTENT_FLEX(source, dispose, copy, PTR_STRUCT_A_S_PTR_GET_SIZE_OF);
}
static void PTR_STRUCT_A_S_PTR_FREE(PTR_STRUCT_A_S_PTR *t)
{
    if (t == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid arg "
                                   "PTR_STRUCT_A_S_PTR"
                                   "* t=%p",
                                   t));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "invalid arg "
                       "PTR_STRUCT_A_S_PTR"
                       "* t=%p",
                       t);
        } while (0);
    }
    else
    {
        PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl =
            ((PTR_STRUCT_A_S_PTR_WRAPPER *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_A_S_PTR_WRAPPER *)0)->data)));
        handle_impl->free_function(handle_impl);
    }
}
static void PTR_STRUCT_A_S_PTR_DEC_REF(CONST_P2_CONST_PTR_STRUCT_A_S_PTR t)
{
    PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl =
        ((PTR_STRUCT_A_S_PTR_WRAPPER *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_A_S_PTR_WRAPPER *)0)->data)));
    if (interlocked_decrement(&handle_impl->refCount) == 0)
    {
        if (handle_impl->dispose != ((void *)0))
        {
            handle_impl->dispose(&handle_impl->data);
        }
        handle_impl->free_function(handle_impl);
    }
}
static void PTR_STRUCT_A_S_PTR_INC_REF(CONST_P2_CONST_PTR_STRUCT_A_S_PTR t)
{
    PTR_STRUCT_A_S_PTR_WRAPPER *handle_impl =
        ((PTR_STRUCT_A_S_PTR_WRAPPER *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_A_S_PTR_WRAPPER *)0)->data)));
    (void)interlocked_increment(&handle_impl->refCount);
}
void PTR_STRUCT_A_S_PTR_ASSIGN(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR t2)
{
    if (t1 == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_A_S_PTR"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_A_S_PTR"
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
                PTR_STRUCT_A_S_PTR_INC_REF(t2);
                *(PTR_STRUCT_A_S_PTR const **)t1 = t2;
            }
        }
        else
        {
            if (t2 == ((void *)0))
            {
                PTR_STRUCT_A_S_PTR_DEC_REF(*t1);
                *(PTR_STRUCT_A_S_PTR const **)t1 = t2;
            }
            else
            {
                PTR_STRUCT_A_S_PTR_INC_REF(t2);
                PTR_STRUCT_A_S_PTR_DEC_REF(*t1);
                *(PTR_STRUCT_A_S_PTR const **)t1 = t2;
            }
        }
    }
}
void PTR_STRUCT_A_S_PTR_INITIALIZE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *lvalue, CONST_P2_CONST_PTR_STRUCT_A_S_PTR rvalue)
{
    if (lvalue == ((void *)0))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") * lvalue=%p, THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") rvalue=%p",
                                   lvalue, rvalue));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_A_S_PTR"
                       ") * lvalue=%p, THANDLE("
                       "PTR_STRUCT_A_S_PTR"
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
            PTR_STRUCT_A_S_PTR_INC_REF(rvalue);
        }
        *(PTR_STRUCT_A_S_PTR const **)lvalue = rvalue;
    }
}
static PTR_STRUCT_A_S_PTR *PTR_STRUCT_A_S_PTR_GET_T(CONST_P2_CONST_PTR_STRUCT_A_S_PTR t)
{
    return (PTR_STRUCT_A_S_PTR *)t;
}
const PTR_STRUCT_A_S_PTR_WRAPPER *const PTR_STRUCT_A_S_PTR_INSPECT(CONST_P2_CONST_PTR_STRUCT_A_S_PTR t)
{
    return ((PTR_STRUCT_A_S_PTR_WRAPPER *)((PCHAR)(t) - (ULONG_PTR)(&((PTR_STRUCT_A_S_PTR_WRAPPER *)0)->data)));
}
void PTR_STRUCT_A_S_PTR_MOVE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t2)
{
    if ((t1 == ((void *)0)) || (t2 == ((void *)0)))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_A_S_PTR"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_A_S_PTR"
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
                *(PTR_STRUCT_A_S_PTR const **)t1 = *t2;
                *(PTR_STRUCT_A_S_PTR const **)t2 = ((void *)0);
            }
        }
        else
        {
            if (*t2 == ((void *)0))
            {
                PTR_STRUCT_A_S_PTR_DEC_REF(*t1);
                *(PTR_STRUCT_A_S_PTR const **)t1 = ((void *)0);
            }
            else
            {
                PTR_STRUCT_A_S_PTR_DEC_REF(*t1);
                *(PTR_STRUCT_A_S_PTR const **)t1 = *t2;
                *(PTR_STRUCT_A_S_PTR const **)t2 = ((void *)0);
            }
        }
    }
}
void PTR_STRUCT_A_S_PTR_INITIALIZE_MOVE(CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t1, CONST_P2_CONST_PTR_STRUCT_A_S_PTR *t2)
{
    if ((t1 == ((void *)0)) || (t2 == ((void *)0)))
    {
        do
        {
            do
            {
                (void)(0 && printf("invalid argument THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") * t1=%p, THANDLE("
                                   "PTR_STRUCT_A_S_PTR"
                                   ") t2=%p",
                                   t1, t2));
            } while (0);
            logger_log(LOG_LEVEL_ERROR, ((void *)0), "D:\\r\\c-pal\\common\\tests\\thandle_int\\thandle_int.c",
                       __FUNCTION__, 32,
                       "invalid argument THANDLE("
                       "PTR_STRUCT_A_S_PTR"
                       ") * t1=%p, THANDLE("
                       "PTR_STRUCT_A_S_PTR"
                       ") t2=%p",
                       t1, t2);
        } while (0);
    }
    else
    {
        if (*t2 == ((void *)0))
        {
            *(PTR_STRUCT_A_S_PTR const **)t1 = ((void *)0);
        }
        else
        {
            *(PTR_STRUCT_A_S_PTR const **)t1 = *t2;
            *(PTR_STRUCT_A_S_PTR const **)t2 = ((void *)0);
        }
    }
};
CONST_P2_CONST_PTR_STRUCT_A_S_PTR create(A_S_PTR source)
{
    PTR_STRUCT_A_S_PTR temp = {.pointer = source};
    return A_S_PTR_CREATE_FROM_CONTENT(temp, ((void *)0), ((void *)0));
};
#endif

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
}

TEST_FUNCTION_CLEANUP(cleans)
{
}



TEST_FUNCTION(thandle_int_works)
{
    ///arrange
    A_S_PTR a_s = malloc(sizeof(A_S));
    ASSERT_IS_NOT_NULL(a_s);
    a_s->a = 42;
    a_s->s = sprintf_char("3333333333333333333333_here_some_string_3333333333333333333333");
    ASSERT_IS_NOT_NULL(a_s->s);


    ///act
    THANDLE(PTR_STRUCT_TYPE_NAME(A_S_PTR)) one = create(a_s);

    ///assert
    ASSERT_IS_NOT_NULL(one);
    ASSERT_ARE_EQUAL(void_ptr, a_s, one->pointer);

    ///cleanup
    THANDLE_ASSIGN(PTR_STRUCT_TYPE_NAME(A_S_PTR))(&one, NULL);
    free(a_s);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

