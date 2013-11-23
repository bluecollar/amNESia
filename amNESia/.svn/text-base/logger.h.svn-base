#pragma once

#include <stdarg.h> // va_start(), va_*
#include <stdio.h>  // printf()
#include <time.h>   // ctime()

extern "C" int g_log_debug;


namespace amnesia 
{
class Logger;

void lognow(const char *format, ...);
#define logdbg(fmt, ...) if (g_log_debug) { lognow(fmt, __VA_ARGS__); }

}// end of namespace amnesia