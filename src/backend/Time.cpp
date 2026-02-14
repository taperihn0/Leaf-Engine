#include "Time.hpp"
#include "Search.hpp"

timepoint_t Clock::timePoint() { 
	return internal_clock_t::now();
}

time_ms_t Clock::getMilliseconds(timepoint_t stop, timepoint_t start) { 
	return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count(); 
}

void Timer::go() {
	_start_tp = Clock::timePoint();
}

time_ms_t Timer::duration() const {
	return Clock::getMilliseconds(Clock::timePoint(), _start_tp);
}

time_ms_t TimeMan::searchTime(const Position& pos, SearchLimits& limits) {
	return pos.getTurn() == WHITE ? (limits.wtime / 20 + limits.winc / 2)
								  : (limits.btime / 20 + limits.binc / 2);
}