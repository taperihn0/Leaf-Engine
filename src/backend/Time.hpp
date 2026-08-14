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

namespace search { class SearchLimits; };
class Position;

namespace clk {

using milliseconds = ll;
using seconds = ll;
/* Main backend clock is `std::chrono::steady_clock`
*/
using backend_clock = std::chrono::steady_clock;
using time_point = backend_clock::time_point;

class Clock {
public:
    _NODISCARD static const Clock& getInstance();
    _NODISCARD time_point getTimePoint() const;
    _NODISCARD milliseconds getMilliseconds(time_point stop, time_point start) const;
private:
    Clock() = default;
};

class Timer {
public:
    Timer() = default;
    void go();
    void reset();
    _NODISCARD milliseconds getDurationMs() const;
private:
    bool       _run = false;
    time_point _start_tp;
};

class TimeManager {
public:
    TimeManager() = delete;
    _NODISCARD static milliseconds searchTimeMs(const Position& pos, const search::SearchLimits& limits);
};

} // namespace clk

constexpr clk::milliseconds operator"" _ms(ull t) noexcept {
    return static_cast<clk::milliseconds>(t);
}

constexpr clk::seconds operator"" _s(ull t) noexcept {
    return static_cast<clk::seconds>(t);
}
