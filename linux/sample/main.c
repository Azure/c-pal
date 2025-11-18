// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include "valgrind/helgrind.h"

#include "c_pal/interlocked.h"


static volatile _Atomic int32_t g_continue_running = 0;
static volatile _Atomic int32_t value = 0;
//static uint32_t value = 0;

static void my_sleep(unsigned int milliseconds)
{
    time_t seconds = milliseconds / 1000;
    long nsRemainder = (milliseconds % 1000) * 1000000;
    struct timespec timeToSleep = { seconds, nsRemainder };
    (void)nanosleep(&timeToSleep, NULL);
}

static void* running_thread(void* thread_ctx)
{
    do
    {
        interlocked_add(&value, 1);
        my_sleep(10);
    } while (interlocked_add(&g_continue_running, 0) != 0);
    return (void*)(intptr_t)1;
}

int main(void)
{
    printf("Starting test\n");
    pthread_t pthread_handle;

    printf("Starting thread function\n");
    if (pthread_create(&pthread_handle, NULL, running_thread, NULL) != 0)
    {
        printf("Failure creating thread\n");
    }
    else
    {
        do
        {
            interlocked_add(&value, 1);
            if (value >= 1000)
            {
                printf("Value is a %" PRIu32 "\n", value);
                interlocked_add(&g_continue_running, 1);
                break;
            }
            my_sleep(50);
        } while (true);

        void* threadResult;
        (void)pthread_join(pthread_handle, &threadResult);

        printf("Test is complete\n");
    }
    return 0;
}
