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

#include "Time.hpp"
#include "Search.hpp"
#include "Position.hpp"

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