#include "crash_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>

// Global crash handler for debugging
void crash_handler(int sig) {
    void *array[20];
    size_t size;

    printf("[E] CRASH: Signal %d detected\n", sig);

    // Get void*'s for all entries on the stack
    size = backtrace(array, 20);

    // Print out all the frames to stderr
    printf("[E] CRASH: Stack trace:\n");
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    // Also log to console
    char **strings = backtrace_symbols(array, size);
    if (strings != NULL) {
        for (size_t i = 0; i < size; i++) {
            printf("[E] CRASH:   %s\n", strings[i]);
        }
        free(strings);
    }

    printf("[E] CRASH: End crash dump\n");
    exit(1);
}

// Initialize crash handlers for common signals
void crash_handler_init(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGBUS, crash_handler);
    printf("[I] crash_handler: Crash handlers initialized\n");
}
