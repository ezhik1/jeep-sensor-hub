#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <signal.h>

// Global crash handler for debugging
void crash_handler(int sig);

// Initialize crash handlers for common signals
void crash_handler_init(void);

#endif // CRASH_HANDLER_H
