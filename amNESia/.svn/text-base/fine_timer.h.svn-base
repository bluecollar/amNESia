#pragma once

#include <assert.h>


class FineTimer
{
public:
	FineTimer() 
	{
		BOOL bRet = QueryPerformanceFrequency(&_hostTimerFrequency);
		assert( bRet );
	}
	~FineTimer() {}

	// TODO - take reciprocal of frequency so we can eliminate
	// the costly divide

	void start()
	{
		QueryPerformanceCounter(&_lastTimer);
		_hostTimer.QuadPart = _lastTimer.QuadPart;
	}
	
	inline LONGLONG measure()
	{
		QueryPerformanceCounter(&_hostTimer);
		return _hostTimer.QuadPart - _lastTimer.QuadPart;
	}

	/*LONGLONG stop()
	{
		LONGLONG final; q	
		QueryPerformanceCounter(&_hostTimer);
		final = _hostTimer.QuadPart - _lastTimer.QuadPart;
		reset();
		return final;
	}*/

	inline void reset()  { _hostTimer.QuadPart = _lastTimer.QuadPart  = 0; }

	inline LONGLONG getFrequency() { return _hostTimerFrequency.QuadPart; }
	inline double getSecondsElapsed() { return (measure() / (double)getFrequency()); }
	inline double getMillisecondsElapsed() { return 1000.f * (measure() / (double)getFrequency()); }

private:
	LARGE_INTEGER _lastTimer;
	LARGE_INTEGER _hostTimer;
	LARGE_INTEGER _hostTimerFrequency;
};