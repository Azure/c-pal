// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef PROCESS_WATCHDOG_INT_COMMON_H
#define PROCESS_WATCHDOG_INT_COMMON_H

// Exit codes for process_watchdog integration test child process
// These are shared between the parent test and child process

#define CHILD_EXIT_CODE_SUCCESS 0
#define CHILD_EXIT_CODE_INIT_FAILED 1
#define CHILD_EXIT_CODE_SURVIVED_TIMEOUT 2

#endif // PROCESS_WATCHDOG_INT_COMMON_H
