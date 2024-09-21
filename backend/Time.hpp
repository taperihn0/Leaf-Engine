#pragma once

#include "Common.hpp"
#include "Position.hpp"

#include <chrono>

class Timer {
public:
	void go();
	void stop();
	auto duration();
private:
	auto now();

	std::chrono::system_clock::time_point _start, _stop;
};

INLINE auto Timer::now() {
	static std::chrono::system_clock clock;
	return clock.now();
}

INLINE void Timer::go() {
	_start = now();
}

INLINE void Timer::stop() {
	_stop = now();
}

INLINE auto Timer::duration() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start).count();
}

struct SearchLimits;

class TimeMan {
public:
	static unsigned searchTime(const Position& pos, SearchLimits& limits);
};