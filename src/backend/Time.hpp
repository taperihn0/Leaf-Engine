#pragma once

#include "Common.hpp"
#include "Position.hpp"

#include <chrono>

using time_ms_t = ll;
using internal_clock_t = std::chrono::steady_clock;
using timepoint_t = internal_clock_t::time_point;

INLINE constexpr time_ms_t operator"" _ms(ull t) {
	return static_cast<time_ms_t>(t);
}

INLINE constexpr time_ms_t operator"" _s(ull t) {
	return static_cast<time_ms_t>(t) * 1000;
}

class Clock {
public:
	Clock() = delete;
	static timepoint_t timePoint();
	static time_ms_t getMilliseconds(timepoint_t stop, timepoint_t start);
};

class Timer {
public:
	void go();
	time_ms_t duration() const;
private:
	timepoint_t _start_tp;
};

struct SearchLimits;

class TimeMan {
public:
	TimeMan() = delete;
	static time_ms_t searchTime(const Position& pos, SearchLimits& limits);
};