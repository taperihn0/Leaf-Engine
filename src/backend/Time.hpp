#pragma once

#include "Common.hpp"
#include "Position.hpp"

#include <chrono>

using time_ms_t = ll;
using timepoint_t = std::chrono::system_clock::time_point;

class Clock {
public:
	static timepoint_t timePoint();
	static time_ms_t getMilliseconds(timepoint_t stop, timepoint_t start);
private:
	using _internal_clock_t = std::chrono::system_clock;
	static _internal_clock_t _clock_data;
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
	static time_ms_t searchTime(const Position& pos, SearchLimits& limits);
};