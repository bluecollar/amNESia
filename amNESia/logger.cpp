#include <cstdarg> // va_start(), va_*
#include <sstream> // stringstream

#include <stdio.h>  // printf()
#include <string.h> // strcat_s
#include <time.h>   // ctime()

#include "logger.h"
using namespace amnesia;

int g_log_debug = 0;

void amnesia::lognow(const char *format, ...)
{
	va_list v;
	char buf[4096], *pbuf = buf;
	int n;
	
	// print time
	*(pbuf++) = '[';
	time_t clock = time(0); // this *will* be slow. TODO: don't call the system for time
	ctime_s(pbuf, 26, &clock);
	pbuf+=24;
	*(pbuf++) = ']';
	*(pbuf++) = ' ';

	va_start(v, format);
	n = _vsnprintf_s(pbuf, sizeof(buf) - (32 + 3), _TRUNCATE, format, v); // 32 for time, 3 for CR/LF/NULL
	va_end(v);

	pbuf += (n < 0) ? sizeof(buf) - (32 + 3) : n;
	*(pbuf++) = '\r';
	*(pbuf++) = '\n';
	*(pbuf++) = '\0';

	OutputDebugStringA(buf);
}

/*class Logger
{
public:
	typedef enum log_severity_t 
	{
		UNKNOWN = 0,
		TRACE   = 1,
		DEBUG   = 2,
		WARN    = 3,
		ERROR   = 4,
		FATAL   = 5,
		NUM_TYPES
	};

	typedef enum log_destination_t
	{
		STDERR,
		STDOUT,
		CONSOLE,
		LOGFILE
	};

	void log(log_destination_t d, log_severity_t s, char* msg, ...)
	
};*/