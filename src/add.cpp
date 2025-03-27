#include <stdio.h>
#include "add.h"

int add(int a, int b) {
    return a + b;
}

void system() {
#if defined(__x86_64__) || defined(__amd64__)
    printf("This is an x86_64 system.\n");
#elif defined(__i386__)
    printf("This is an x86 32-bit system.\n");
#elif defined(__arm__)
    printf("This is an ARM system.\n");
#elif defined(__aarch64__)
    printf("This is an ARM64 system.\n");
#elif defined(__powerpc__)
    printf("This is a PowerPC system.\n");
#elif defined(__mips__)
    printf("This is a MIPS system.\n");
#else
    printf("Architecture is unknown.\n");
#endif
}
