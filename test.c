#include <sys/syscall.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


void main()
{
    volatile _Atomic int a = 1;
    struct timespec timeout = {0,500};
    printf("%ld", syscall(SYS_futex, &a, FUTEX_WAIT, 1, &timeout, NULL, 0));
    printf("wait complete");
    // futex(&a, FUTEX_WAIT, 0, NULL, NULL, 0);
}