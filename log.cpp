#include "log.h"

#include <stdio.h>
#include <stdarg.h>

void log_print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void log_println(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);

    fflush(stdout);
}

#if if_debug
void debug_println(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);

    fflush(stdout);
}
#endif