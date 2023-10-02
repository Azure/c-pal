// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"

#include "pdh.h"

#include "c_pal/single_performance_counter.h"

typedef struct SINGLE_PERFORMANCE_COUNTER_TAG
{
    PDH_HQUERY perf_query_handle;
    HCOUNTER current_counter;
} SINGLE_PERFORMANCE_COUNTER;

MU_DEFINE_ENUM_STRINGS(SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT_VALUES);

static int get_current_app_name(char* app_name, uint32_t app_name_size)
{
    int result;
    char executable_full_file_name[MAX_PATH + 1];
    // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_006: [ single_performance_counter_create shall call GetModuleFileNameA to get the current executable path. ]
    DWORD get_module_file_name_a_result = GetModuleFileNameA(NULL, executable_full_file_name, MAX_PATH + 1);

    if (get_module_file_name_a_result == MAX_PATH + 1)
    {
        LogLastError("failure in GetModuleFileNameA(NULL, fullExecutableFileName=%p, MAX_PATH=%d + 1), it returned %" PRIu32 " which means location did not have enough space to hold the path", executable_full_file_name, MAX_PATH, get_module_file_name_a_result);
        result = MU_FAILURE;
    }
    else if (get_module_file_name_a_result == 0)
    {
        LogLastError("failure in GetModuleFileNameA(NULL, fullExecutableFileName=%p, MAX_PATH=%d + 1), it returned %" PRIu32 "", executable_full_file_name, MAX_PATH, get_module_file_name_a_result);
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_007: [ single_performance_counter_create shall call strrchr on the executable path with '\' to get the executable name. ]
        const char* where_is_last_backslash = strrchr(executable_full_file_name, '\\');
        if (where_is_last_backslash == NULL)
        {
            LogError("unexpected not to have a \\ character in fullExecutableFileName=%s", executable_full_file_name);
            result = MU_FAILURE;
        }
        else
        {
            // Move past last backslash.
            where_is_last_backslash++;
            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_008: [ single_performance_counter_create shall call strrchr on the executable name with '.' to get the executable name without extension. ]
            const char* where_is_last_dot = strrchr(where_is_last_backslash, '.');
            if (where_is_last_dot == NULL)
            {
                LogError("unexpected not to have a . character in where_is_last_backslash=%s", where_is_last_backslash);
                result = MU_FAILURE;
            }
            else if (where_is_last_dot <= where_is_last_backslash)
            {
                LogError("unexpected not to have a . character in where_is_last_backslash=%s", where_is_last_backslash);
                result = MU_FAILURE;
            }
            else
            {
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_009: [ single_performance_counter_create shall call snprintf to copy the executable name without extension. ]
                int bytes_printed = snprintf(app_name, app_name_size, "%*.*s", 
                    (int)(where_is_last_dot - where_is_last_backslash), (int)(where_is_last_dot - where_is_last_backslash), where_is_last_backslash);
                if (bytes_printed < 0)
                {
                    LogError("sprintf failure: app_name = %p, size = %" PRIu32 ", where_is_last_backslash = %s, precision = %" PRIi32 "",
                        app_name, app_name_size, where_is_last_backslash, (int)(where_is_last_dot - where_is_last_backslash));
                    result = MU_FAILURE;
                }
                else if ((uint32_t)bytes_printed >= app_name_size - 1)
                {
                    LogError("given buffer for app_name, size = %" PRIu32 "is smaller than found module name where_is_last_backslash = % s", app_name_size, where_is_last_backslash);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}

static int get_current_counter_path_string(const char* performance_object, const char* performance_counter, char* path_string, uint32_t max_path_size)
{
    int result;
    char app_name[MAX_PATH];
    // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_005: [ single_performance_counter_create shall call GetCurrentProcessId to get the current process id. ]
    DWORD pid = GetCurrentProcessId();
    result = get_current_app_name(app_name, MAX_PATH);
    if (result != 0)
    {
        LogError("get_current_app_name failed, result = %" PRIi32 "", result);
    }
    else
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_010: [ single_performance_counter_create shall call snprintf to copy the performance_object, performance_counter, pid, and the executable name without extension into a single string. ]
        int bytes_printed = snprintf(path_string, max_path_size, "\\%s(%s:%d)\\%s",
            performance_object, app_name, pid, performance_counter);
        if (bytes_printed < 0)
        {
            LogError("sprintf failure: path_string = %p, size = %" PRIu32 ", app_name = %s",
                path_string, max_path_size, app_name);
            result = MU_FAILURE;
        }
        else if ((uint32_t)bytes_printed >= max_path_size - 1)
        {
            LogError("given buffer for path_string, size = %" PRIu32 "is smaller than counter path value = %" PRIi32 "", max_path_size, bytes_printed);
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_HANDLE, single_performance_counter_create, const char*, performance_object, const char*, performance_counter)
{
    SINGLE_PERFORMANCE_COUNTER_HANDLE result;
    if 
    ( 
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_002: [ single_performance_counter_create shall return NULL if performance_object is NULL. ]
        performance_object == NULL ||
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_003: [ single_performance_counter_create shall return NULL if performance_counter is NULL. ]
        performance_counter == NULL
        )
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
        LogError("Invalid Arguments: const char* performance_object = %p, const char* performance_counter = %p",
            performance_object, performance_counter);
        result = NULL;
    }
    else
    {
        PDH_HQUERY perf_query_handle;
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_011: [ single_performance_counter_create shall call PdhOpenQueryA to create a new query. ]
        PDH_STATUS perf_status = PdhOpenQueryA(NULL, (DWORD_PTR)NULL, &perf_query_handle);
        if (perf_status != ERROR_SUCCESS)
        {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
            LogError("Failure in PdhOpenQueryA, status = %" PRIx32"", perf_status);
            result = NULL;
        }
        else
        {
            // See: https://learn.microsoft.com/en-us/windows/win32/perfctrs/specifying-a-counter-path
            // Path should be: "\Process V2([app_name]:[pid])\% Processor Time"
            // Getting local machine value, should NOT need "\\[hostname] prefix.
            char counter_path_buffer[MAX_PATH]; // PDH_MAX_COUNTER_PATH is a bit large, we expect this to be < MAX_PATH
            if (get_current_counter_path_string(performance_object, performance_counter, counter_path_buffer, MAX_PATH) != 0)
            {
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
                LogError("Failure in get_current_counter_path_string");
                result = NULL;
            }
            else
            {
                HCOUNTER counter;
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_012: [ single_performance_counter_create shall call PdhAddCounterA to add the counter to the query. ]
                perf_status = PdhAddCounterA(perf_query_handle, counter_path_buffer, 0, &counter);
                if (perf_status != ERROR_SUCCESS)
                {
                    // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
                    LogError("Failure in PdhAddCounterA, status = %" PRIx32"", perf_status);
                    result = NULL;
                }
                else
                {
                    // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_013: [ single_performance_counter_create shall call PdhCollectQueryData to prime the query. ]
                    perf_status = PdhCollectQueryData(perf_query_handle);
                    if (perf_status != ERROR_SUCCESS)
                    {
                        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
                        LogError("Failure in PdhCollectQueryData, status = %" PRIx32"", perf_status);
                        result = NULL;
                    }
                    else
                    {
                        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_004: [ single_performance_counter_create shall call malloc to create a new SINGLE_PERFORMANCE_COUNTER struct. ]
                        result = malloc(sizeof(SINGLE_PERFORMANCE_COUNTER));
                        if (result == NULL)
                        {
                            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
                            LogError("Unable to allocate SINGLE_PERFORMANCE_COUNTER");
                        }
                        else
                        {
                            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_014: [ single_performance_counter_create shall save PDH_HQUERY handle, and counter HCOUNTER in the SINGLE_PERFORMANCE_COUNTER struct. ]
                            result->perf_query_handle = perf_query_handle;
                            result->current_counter = counter;
                            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_015: [ single_performance_counter_create shall return a non-null to the SINGLE_PERFORMANCE_COUNTER_HANDLE struct on success. ]
                            goto all_ok;
                        }
                    }
                }
            }
            PdhCloseQuery(perf_query_handle);
        }
    }
    all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, single_performance_counter_destroy, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle)
{
    if (handle == NULL)
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_017: [ single_performance_counter_destroy shall return if handle is NULL. ]
        LogError("NULL handle");
    }
    else
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_018: [ single_performance_counter_destroy shall call PdhCloseQuery with the PDH_HQUERY handle. ]
        PdhCloseQuery(handle->perf_query_handle);
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_019: [ single_performance_counter_destroy shall call free on the handle ]
        free(handle);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, single_performance_counter_sample_double, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle, double*, sample)
{
    SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result;
    if
    (
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_020: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR if handle is NULL. ]
        handle == NULL ||
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_027: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR if sample is NULL. ]
        sample == NULL
    )
    {
        LogError("Invalid Arguments: SINGLE_PERFORMANCE_COUNTER_HANDLE handle = %p, double* sample = %p",
            handle, sample);
        result = SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR;
    }
    else
    {
        // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_021: [ single_performance_counter_sample_double shall call PdhCollectQueryData on the PDH_HQUERY handle. ]
        PDH_STATUS perf_status = PdhCollectQueryData(handle->perf_query_handle);
        if (perf_status != ERROR_SUCCESS)
        {
            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_022: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_COLLECT_FAILED if PdhCollectQueryData fails. ]
            LogError("Failure in PdhCollectQueryData, status = %" PRIx32 "", perf_status);
            result = SINGLE_PERFORMANCE_COUNTER_SAMPLE_COLLECT_FAILED;
        }
        else
        {
            PDH_FMT_COUNTERVALUE formatted_value;
            DWORD counter_type;
            // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_023: [ single_performance_counter_sample_double shall call PdhGetFormattedCounterValue on the counter HCOUNTER. ]
            perf_status = PdhGetFormattedCounterValue(handle->current_counter,
                PDH_FMT_DOUBLE,
                &counter_type,
                &formatted_value);
            if (perf_status != ERROR_SUCCESS)
            {
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_024: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_FORMAT_FAILED if PdhGetFormattedCounterValue fails. ]
                LogError("Failure in PdhGetFormattedCounterValue, status = %" PRIx32"", perf_status);
                result = SINGLE_PERFORMANCE_COUNTER_SAMPLE_FORMAT_FAILED;
            }
            else
            {
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_026: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS on success. ]
                result = SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS;
                // Codes_SRS_SINGLE_PERFORMANCE_COUNTER_45_025: [ single_performance_counter_sample_double shall set the sample to the double value given by PdhGetFormattedCounterValue. ]
                *sample = formatted_value.doubleValue;
            }
        }
    }
    return result;
}


