#include "logger.h"

#include <cstdarg>     // va_start(), va_*
#include <stdio.h>     // _vsnprintf_s()
#include <time.h>      // time(), ctime_s()
#include <windows.h>   // OutputDebugStringA


Logger g_logger;


void Logger::log(const char *format, ...)
{
	// Abide the rare request to squelch any iota of help come crash time
	if (_log_level == L_QUIET)
		return;

	va_list va;
	va_start(va, format);
		vlog(format, va);
	va_end(va);
}

void Logger::vlog(const char * format, va_list argp)
{
	// Locals
	char buf[4096], *pbuf = buf;
	int n;
	
	// Print time
	*(pbuf++) = '[';
	time_t clock = time(0); // this *will* be slow. TODO: don't call the system for time
	ctime_s(pbuf, 26, &clock);
	pbuf+=24;
	*(pbuf++) = ']';
	*(pbuf++) = ' ';

	n = _vsnprintf_s(pbuf, sizeof(buf) - (32 + 3), _TRUNCATE, format, argp); // 32 for time, 3 for CR/LF/NULL

	pbuf += (n < 0) ? sizeof(buf) - (32 + 3) : n;
	*(pbuf++) = '\r';
	*(pbuf++) = '\n';
	*(pbuf++) = '\0';

	OutputDebugStringA(buf);
}


void Logger::logDebug(const char *format, ...)
{
	if (!showLevel(L_DEBUG))
		return;

	va_list varg;
	va_start(varg, format);
		vlog(format, varg);
	va_end(varg);
}

void Logger::logTrace(const char *format, ...)
{
	if (!showLevel(L_TRACE))
		return;

	va_list varg;
	va_start(varg, format);
		vlog(format, varg);
	va_end(varg);
}

void Logger::logError(const char *format, ...)
{
	if (!showLevel(L_ERROR))
		return;

	va_list varg;
	va_start(varg, format);
		vlog(format, varg);
	va_end(varg);
}

void Logger::setLevel(log_level_t level)
{ 
	if (level == L_END) 
		level = L_QUIET;

	if (_log_level != level) {
		log(" *** ");
		log("changing logging level from %d to %d", _log_level, level);
		log(" *** ");
		_log_level = level; 
	}
}

