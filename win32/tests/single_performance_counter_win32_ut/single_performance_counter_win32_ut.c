// Copyright (c) Microsoft. All rights reserved.


#include "single_performance_counter_win32_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(WINAPI, DWORD, mocked_GetCurrentProcessId);

MOCKABLE_FUNCTION(WINAPI, DWORD, mocked_GetModuleFileNameA, HMODULE, hModule, LPSTR, lpFilename, DWORD, nSize);

MOCKABLE_FUNCTION(WINAPI, PDH_STATUS, mocked_PdhOpenQueryA, LPCSTR, szDataSource, DWORD*, dwUserData, PDH_HQUERY*, phQuery);

MOCKABLE_FUNCTION(WINAPI, PDH_STATUS, mocked_PdhAddCounterA, PDH_HQUERY, hQuery, LPCSTR, szFullCounterPath, DWORD*, dwUserData, PDH_HCOUNTER*, phCounter);

MOCKABLE_FUNCTION(WINAPI, PDH_STATUS, mocked_PdhCollectQueryData, PDH_HQUERY, hQuery);

MOCKABLE_FUNCTION(WINAPI, PDH_STATUS, mocked_PdhCloseQuery, PDH_HQUERY, hQuery);

MOCKABLE_FUNCTION(WINAPI, PDH_STATUS, mocked_PdhGetFormattedCounterValue, PDH_HCOUNTER, hCounter, DWORD, dwFormat, DWORD*, lpdwType, PPDH_FMT_COUNTERVALUE, pValue);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

IMPLEMENT_UMOCK_C_ENUM_TYPE(SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT_VALUES);

//
// Hooks for system calls
//

#define DEFAULT_MOCKED_GETCURRENTPROCESSID_PID 1;
static DWORD mocked_GetCurrentProcessId_pid = DEFAULT_MOCKED_GETCURRENTPROCESSID_PID;
static DWORD hook_mocked_GetCurrentProcessId(void)
{
    return mocked_GetCurrentProcessId_pid;
}

#define DEFAULT_MOCKED_GETMODULEFILENAMEA_FILENAME "c:\\test\\Azure.Messaging.ElasticLog.exe";
static char* mocked_mocked_GetModuleFileNameA_filename = DEFAULT_MOCKED_GETMODULEFILENAMEA_FILENAME;
static DWORD hook_mocked_GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
    (void)hModule;
    (void)nSize;
    DWORD result = (DWORD)((mocked_mocked_GetModuleFileNameA_filename != NULL) ? strlen(mocked_mocked_GetModuleFileNameA_filename) : 0);
    if (result >= MAX_PATH + 1)
    {
        result = MAX_PATH + 1;
    }
    else if (result != 0)
    {
        strcpy(lpFilename, mocked_mocked_GetModuleFileNameA_filename);
    }
    return result;
}

static PDH_STATUS hook_mocked_PdhOpenQueryA(LPCSTR szDataSource, DWORD* dwUserData, PDH_HQUERY* phQuery)
{
    (void)szDataSource;
    (void)dwUserData;
    *phQuery = (PDH_HQUERY)real_gballoc_hl_malloc(1);
    (void)phQuery;
    return ERROR_SUCCESS;
}

static PDH_STATUS hook_mocked_PdhAddCounterA(PDH_HQUERY hQuery, LPCSTR szFullCounterPath, DWORD* dwUserData, PDH_HCOUNTER* phCounter)
{
    (void)hQuery;
    (void)szFullCounterPath;
    (void)dwUserData;
    (void)phCounter;
    return ERROR_SUCCESS;
}

static PDH_STATUS hook_mocked_PdhCollectQueryData(PDH_HQUERY hQuery)
{
    (void)hQuery;
    return ERROR_SUCCESS;
}

static PDH_STATUS hook_mocked_PdhCloseQuery(PDH_HQUERY hQuery)
{
    real_gballoc_hl_free((void*)hQuery);
    return ERROR_SUCCESS;
}

static double mocked_PdhGetFormattedCounterValue_pValue_double = 0.1;
static PDH_STATUS hook_mocked_PdhGetFormattedCounterValue(PDH_HCOUNTER hCounter, DWORD dwFormat, DWORD* lpdwType, PPDH_FMT_COUNTERVALUE pValue)
{
    (void)hCounter;
    (void)dwFormat;
    (void)lpdwType;
    if (pValue != NULL)
    {
        pValue->doubleValue = mocked_PdhGetFormattedCounterValue_pValue_double;
    }
    return ERROR_SUCCESS;
}

static void default_single_performance_counter_create_mocks()
{
    // "performance_object", "performance_counter"
    // default path: "c:\\test\\Azure.Messaging.ElasticLog.exe", default pid = 1
    char * default_expected_pdh_path = "\\performance_object(Azure.Messaging.ElasticLog:1)\\performance_counter";
    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId());
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, MAX_PATH + 1));
    STRICT_EXPECTED_CALL(mocked_PdhAddCounterA(IGNORED_ARG, default_expected_pdh_path, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCollectQueryData(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
}

static SINGLE_PERFORMANCE_COUNTER_HANDLE create_default_single_performance_counter()
{
    default_single_performance_counter_create_mocks();
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    return result;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(SINGLE_PERFORMANCE_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDH_HQUERY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HCOUNTER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDH_HCOUNTER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HMODULE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PPDH_FMT_COUNTERVALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPSTR, char*);
    REGISTER_UMOCK_ALIAS_TYPE(LPCSTR, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, uint32_t);
    REGISTER_UMOCK_ALIAS_TYPE(PDH_STATUS, int);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();

    REGISTER_TYPE(SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_GetModuleFileNameA, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_PdhOpenQueryA, ERROR_BAD_FORMAT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_PdhAddCounterA, ERROR_BAD_FORMAT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_PdhCollectQueryData, ERROR_BAD_FORMAT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_PdhGetFormattedCounterValue, ERROR_BAD_FORMAT);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_GetModuleFileNameA, hook_mocked_GetModuleFileNameA);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_GetCurrentProcessId, hook_mocked_GetCurrentProcessId);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_PdhOpenQueryA, hook_mocked_PdhOpenQueryA);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_PdhAddCounterA, hook_mocked_PdhAddCounterA);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_PdhCollectQueryData, hook_mocked_PdhCollectQueryData);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_PdhCloseQuery, hook_mocked_PdhCloseQuery);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_PdhGetFormattedCounterValue, hook_mocked_PdhGetFormattedCounterValue);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
    // reset the mocked value returns to default
    mocked_mocked_GetModuleFileNameA_filename = DEFAULT_MOCKED_GETMODULEFILENAMEA_FILENAME;
    mocked_GetCurrentProcessId_pid = DEFAULT_MOCKED_GETCURRENTPROCESSID_PID;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// single_performance_counter_create
//

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_002: [ single_performance_counter_create shall return NULL if performance_object is NULL. ]
TEST_FUNCTION(single_performance_counter_create_returns_null_if_performance_object_is_null)
{
    // arrange

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create(NULL, "performance_counter");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_003: [ single_performance_counter_create shall return NULL if performance_counter is NULL. ]
TEST_FUNCTION(single_performance_counter_create_returns_null_if_performance_counter_is_null)
{
    // arrange

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // ablution
}


// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_004 : [single_performance_counter_create shall call malloc to create a new SINGLE_PERFORMANCE_COUNTER struct.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_005 : [single_performance_counter_create shall call GetCurrentProcessId to get the current process id.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_006 : [single_performance_counter_create shall call GetModuleFileNameA to get the current executable path.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_007 : [single_performance_counter_create shall call strrchr on the executable path with '\' to get the executable name. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_008 : [single_performance_counter_create shall call strrchr on the executable name with '.' to get the executable name without extension.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_009 : [single_performance_counter_create shall call snprintf to copy the executable name without extension.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_010 : [single_performance_counter_create shall call snprintf to copy the performance_object, performance_counter, pid, and the executable name without extension into a single string.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_011 : [single_performance_counter_create shall call PdhOpenQueryA to create a new query.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_012 : [single_performance_counter_create shall call PdhAddCounterA to add the counter to the query.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_013 : [single_performance_counter_create shall call PdhCollectQueryData to prime the query.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_014 : [single_performance_counter_create shall save PDH_HQUERY handle, and counter HCOUNTER in the SINGLE_PERFORMANCE_COUNTER struct.]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_015 : [single_performance_counter_create shall return a non - null to the SINGLE_PERFORMANCE_COUNTER_HANDLE struct on success.]
TEST_FUNCTION(single_performance_counter_create_succeeds)
{
    // arrange
    default_single_performance_counter_create_mocks();

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    single_performance_counter_destroy(result);
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
TEST_FUNCTION(single_performance_counter_create_negative_tests)
{
    // arrange
    PDH_HQUERY* perf_query_handle;
    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG)).CaptureArgumentValue_phQuery(&perf_query_handle);
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId()).CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhAddCounterA(IGNORED_ARG, IGNORED_ARG, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCollectQueryData(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    size_t i = 0;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            // act
            SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

            // assert
            ASSERT_IS_NULL(result);

            // ablution
        }
    }
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
TEST_FUNCTION(single_performance_counter_create_module_name_too_large)
{
    // arrange
    char new_mocked_name[MAX_PATH + 2];
    memset(new_mocked_name, 'a', MAX_PATH + 2); 
    new_mocked_name[0] = '\\'; // slash to make it a full path
    new_mocked_name[MAX_PATH + 1] = '\0'; // null terminate
    mocked_mocked_GetModuleFileNameA_filename = new_mocked_name;

    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId());
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCloseQuery(IGNORED_ARG));

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_007 : [single_performance_counter_create shall call strrchr on the executable path with '\' to get the executable name. ]
TEST_FUNCTION(single_performance_counter_create_app_name_unexpectedly_not_full_path)
{
    // arrange
    char* new_mocked_name = "name_without_a_slash.exe";
    mocked_mocked_GetModuleFileNameA_filename = new_mocked_name;

    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId());
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCloseQuery(IGNORED_ARG));

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_008 : [single_performance_counter_create shall call strrchr on the executable name with '.' to get the executable name without extension.]
TEST_FUNCTION(single_performance_counter_create_app_name_unexpectedly_has_no_extension)
{
    // arrange
    char* new_mocked_name = "\\name_without_extension";
    mocked_mocked_GetModuleFileNameA_filename = new_mocked_name;

    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId());
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCloseQuery(IGNORED_ARG));

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_016: [ single_performance_counter_create shall return NULL if any step fails. ]
TEST_FUNCTION(single_performance_counter_create_module_name_makes_perf_path_too_large)
{
    // arrange
    char new_mocked_name[MAX_PATH + 1];
    memset(new_mocked_name, 'a', MAX_PATH + 1);
    new_mocked_name[0] = '\\'; // slash to make it a full path
    new_mocked_name[MAX_PATH -1] = '.'; // dot to give it an extension
    new_mocked_name[MAX_PATH ] = '\0'; // null terminate
    mocked_mocked_GetModuleFileNameA_filename = new_mocked_name;

    STRICT_EXPECTED_CALL(mocked_PdhOpenQueryA(NULL, 0, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcessId());
    STRICT_EXPECTED_CALL(mocked_GetModuleFileNameA(NULL, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhCloseQuery(IGNORED_ARG));

    // act
    SINGLE_PERFORMANCE_COUNTER_HANDLE result = single_performance_counter_create("performance_object", "performance_counter");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //ablution
}

//
// single_performance_counter_destroy
//

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_017: [ single_performance_counter_destroy shall return if handle is NULL. ]
TEST_FUNCTION(single_performance_counter_destroy_returns_if_handle_is_null)
{
    // arrange

    // act
    single_performance_counter_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_018: [ single_performance_counter_destroy shall call PdhCloseQuery with the PDH_HQUERY handle. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_019: [ single_performance_counter_destroy shall call free on the handle ]
TEST_FUNCTION(single_performance_counter_destroy_succeeds)
{
    // arrange
    SINGLE_PERFORMANCE_COUNTER_HANDLE handle = create_default_single_performance_counter();

    STRICT_EXPECTED_CALL(mocked_PdhCloseQuery(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    single_performance_counter_destroy(handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

//
// single_performance_counter_sample_double
//

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_020: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR if handle is NULL. ]
TEST_FUNCTION(single_performance_counter_sample_double_returns_error_if_handle_is_null)
{
    // arrange

    // act
    double sample;
    SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result = single_performance_counter_sample_double(NULL, &sample);

    // assert
    ASSERT_ARE_EQUAL(int, SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // ablution
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_027: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR if sample is NULL. ]
TEST_FUNCTION(single_performance_counter_sample_double_returns_error_if_sample_is_null)
{
    // arrange
    SINGLE_PERFORMANCE_COUNTER_HANDLE handle = create_default_single_performance_counter();

    // act
    SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result = single_performance_counter_sample_double(handle, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    single_performance_counter_destroy(handle);
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_021: [ single_performance_counter_sample_double shall call PdhCollectQueryData on the PDH_HQUERY handle. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_023: [ single_performance_counter_sample_double shall call PdhGetFormattedCounterValue on the counter HCOUNTER. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_025: [ single_performance_counter_sample_double shall set the sample to the double value given by PdhGetFormattedCounterValue. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_026: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS on success. ]
TEST_FUNCTION(single_performance_counter_sample_double_succeeds)
{
    // arrange
    SINGLE_PERFORMANCE_COUNTER_HANDLE handle = create_default_single_performance_counter();

    STRICT_EXPECTED_CALL(mocked_PdhCollectQueryData(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhGetFormattedCounterValue(IGNORED_ARG, PDH_FMT_DOUBLE, IGNORED_ARG, IGNORED_ARG));

    // act
    double sample;
    SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result = single_performance_counter_sample_double(handle, &sample);

    // assert
    ASSERT_ARE_EQUAL(int, SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS, result);
    ASSERT_ARE_EQUAL(double, mocked_PdhGetFormattedCounterValue_pValue_double, sample);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    single_performance_counter_destroy(handle);
}

// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_022: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_COLLECT_FAILED if PdhCollectQueryData fails. ]
// Tests_SRS_SINGLE_PERFORMANCE_COUNTER_45_024: [ single_performance_counter_sample_double shall return SINGLE_PERFORMANCE_COUNTER_SAMPLE_FORMAT_FAILED if PdhGetFormattedCounterValue fails. ]
TEST_FUNCTION(single_performance_counter_sample_double_negative_tests)
{
    // arrange
    SINGLE_PERFORMANCE_COUNTER_HANDLE handle = create_default_single_performance_counter();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_PdhCollectQueryData(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_PdhGetFormattedCounterValue(IGNORED_ARG, PDH_FMT_DOUBLE, IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    size_t i = 0;
    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            double sample;
            SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result = single_performance_counter_sample_double(handle, &sample);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, result, SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS);
        }
    }
    // ablution
    single_performance_counter_destroy(handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
