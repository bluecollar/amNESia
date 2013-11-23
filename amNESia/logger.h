// General purpose logging class
// Christopher Bond - 11/2011
#pragma once

#include <cstdarg>

class Logger
{
public:
	Logger() : _log_level(L_ERROR) {}
	~Logger()                      {}

	enum log_level_t {
		L_UNKNOWN  = 0,
		L_QUIET    = 1,
		L_ERROR    = (L_QUIET << 1),
		L_DEBUG    = (L_ERROR << 1),
		L_TRACE    = (L_DEBUG << 1),
		L_END      = (L_TRACE << 1)
	} _log_level;

	/*enum log_level_mask_t {
		L_EMPTY_MASK   = 0,
		L_UNUSED_MASK  = (L_EMPTY<<1) + 1,
		L_TRACE_MASK   = (L_UNUSED<<1) + 1,
		L_DEBUG_MASK   = (L_TRACE<<1) + 1,
		L_ERROR_MASK   = (L_DEBUG<<1) + 1
	};*/

	/*typedef enum log_destination_t
	{
		STDERR,
		STDOUT,
		CONSOLE,
		LOGFILE
	};*/

	void log(const char *format, ...);
	void vlog(const char *format, va_list va);
	//void log(log_level_t level, const char *format, ...);

	void logTrace(const char *format, ...);
	void logDebug(const char *format, ...);
	void logError(const char *format, ...);

	inline void shiftLevel()                  { setLevel((log_level_t)(_log_level << 1)); }
	inline bool showLevel(log_level_t level)  { return (_log_level - level >= 0); }
	void setLevel(log_level_t level);
};

extern Logger g_logger;