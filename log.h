#pragma once

void log_print(const char *format, ...);
void log_println(const char *format, ...);
#define if_debug 0
#if if_debug

void debug_println(const char *format, ...);

#else

#define debug_println 

#endif
