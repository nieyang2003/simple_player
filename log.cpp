#include "log.h"

#include <stdio.h>
#include <strarg.h>

void log_print(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void log_println(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);

    fflush(stdout);
}
