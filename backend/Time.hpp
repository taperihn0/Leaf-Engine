#pragma once

#include <chrono>

class Timer {
public:
	void go();
	void stop();

	// returns time passed in milliseconds
	auto duration();
private:
	inline auto now() {
		static std::chrono::system_clock clock;
		return clock.now();
	}

	std::chrono::system_clock::time_point _start, _stop;
};

inline void Timer::go() {
	_start = now();
}

inline void Timer::stop() {
	_stop = now();
}

inline auto Timer::duration() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start).count();
}