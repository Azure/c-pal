#include "windows.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_pal/execution_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, HANDLE, mock_CreateFileA, LPCSTR, lpFileName, DWORD, dwDesiredAccess, DWORD, dwShareMode, LPSECURITY_ATTRIBUTES, lpSecurityAttributes, DWORD, dwCreationDisposition, DWORD, dwFlagsansAttributes, HANDLE, hTemplateFile);
MOCKABLE_FUNCTION(, BOOL, mock_SetFileCompletionNotificationModes, HANDLE, FileHandle, UCHAR, Flags);
MOCKABLE_FUNCTION(, void, mock_InitializeThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, mock_SetThreadpoolCallbackPool, PTP_CALLBACK_ENVIRON, pcbe, PTP_POOL, ptpp);
MOCKABLE_FUNCTION(, PTP_CLEANUP_GROUP, mock_CreateThreadpoolCleanupGroup);
MOCKABLE_FUNCTION(, void, mock_SetThreadpoolCallbackCleanupGroup, PTP_CALLBACK_ENVIRON, pcbe, PTP_CLEANUP_GROUP, ptpcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK, pfng);
MOCKABLE_FUNCTION(, PTP_IO, mock_CreateThreadpoolIo, HANDLE, fl, PTP_WIN32_IO_CALLBACK, pfnio, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, mock_CloseThreadpoolIo, PTP_IO, pio);



MOCKABLE_FUNCTION(, void, mock_WaitForThreadpoolIoCallbacks, PTP_IO, pio, BOOL, fCancelPendingCallbacks);
MOCKABLE_FUNCTION(, void, mock_CloseThreadpoolCleanupGroup, PTP_CLEANUP_GROUP, ptpcg);
MOCKABLE_FUNCTION(, void, mock_DestroyThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, BOOL, mock_CloseHandle, HANDLE, hObject);

MOCKABLE_FUNCTION(, void, mock_StartThreadpoolIo, PTP_IO, pio);
MOCKABLE_FUNCTION(, HANDLE, mock_CreateEvent, LPSECURITY_ATTRIBUTES, lpEventAttributes, BOOL, bManualReset, BOOL, bInitialState, LPCSTR, lpName);
MOCKABLE_FUNCTION(, BOOL, mock_WriteFile, HANDLE, hFile, LPCVOID, lpBuffer, DWORD, nNumberOfBytesToWrite, LPDWORD, lpNumberOfBytesWritten, LPOVERLAPPED, lpOverlapped);
MOCKABLE_FUNCTION(, BOOL, mock_ReadFile, HANDLE, hFile, LPVOID, lpBuffer, DWORD, nNumberOfBytesToRead, LPDWORD, lpNumberofBytesRead, LPOVERLAPPED, lpOverlapped);
MOCKABLE_FUNCTION(, DWORD, mock_GetLastError);
MOCKABLE_FUNCTION(, void, mock_CancelThreadpoolIo, PTP_IO, pio);
MOCKABLE_FUNCTION(, BOOL, mock_GetOverlappedResult, HANDLE, hFile, LPOVERLAPPED, lpOverlapped, LPDWORD, lpNumberOfBytesTransferred, BOOL, bWait);

#ifdef __cplusplus
}
#endif

