/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "Common.hpp"

#include <chrono>

using time_ms_t = ll;
using time_s_t = ll;
using internal_clock_t = std::chrono::steady_clock;
using timepoint_t = internal_clock_t::time_point;

_INLINE constexpr time_ms_t operator"" _ms(ull t) {
    return static_cast<time_ms_t>(t);
}

_INLINE constexpr time_s_t operator"" _s(ull t) {
    return static_cast<time_ms_t>(t);
}

class Clock {
public:
    Clock() = delete;
    _NODISCARD static timepoint_t timePoint();
    _NODISCARD static time_ms_t getMilliseconds(timepoint_t stop, timepoint_t start);
};

class Timer {
public:
    void go();
    _NODISCARD time_ms_t duration() const;
private:
    timepoint_t _start_tp;
};

struct SearchLimits;
class Position;

class TimeMan {
public:
    TimeMan() = delete;
    _NODISCARD static time_ms_t searchTime(const Position& pos, SearchLimits& limits);
};
