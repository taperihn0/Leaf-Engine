#pragma once

#include "Common.hpp"
#include "Position.hpp"

#include <chrono>

using time_ms_t = ll;
using timepoint_t = std::chrono::system_clock::time_point;

class Clock {
public:
	inline static timepoint_t timePoint()  
	{ return _clock.now(); }

	inline static time_ms_t getMilliseconds(timepoint_t stop, timepoint_t start) 
	{ return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count(); }
private:
	using _internal_clock_t = std::chrono::system_clock;
	static _internal_clock_t _clock;
};

class Timer {
public:
	void go();
	time_ms_t duration();
private:
	timepoint_t _start_tp;
};

INLINE void Timer::go() {
	_start_tp = Clock::timePoint();
}

INLINE time_ms_t Timer::duration() {
	return Clock::getMilliseconds(Clock::timePoint(), _start_tp);
}

struct SearchLimits;

class TimeMan {
public:
	static time_ms_t searchTime(const Position& pos, SearchLimits& limits);
};