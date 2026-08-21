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

namespace clk {

static const Clock ClockInstance = Clock::getInstance();

_NODISCARD const Clock& Clock::getInstance() {
    static Clock ClockInstance;
    return ClockInstance;
}

time_point Clock::getTimePoint() const { 
    return backend_clock::now();
}

milliseconds Clock::getMilliseconds(time_point stop, time_point start) const { 
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count(); 
}

void clk::Timer::go() {
    _start_tp = ClockInstance.getTimePoint();
    _run = true;
}

void clk::Timer::reset() {
    _run = false;
}

milliseconds clk::Timer::getDurationMs() const {
    return _run ? ClockInstance.getMilliseconds(ClockInstance.getTimePoint(), _start_tp) : 0_ms;
}

milliseconds TimeManager::searchTimeMs(const Position& pos, const engine::SearchLimits& limits) {
    return pos.getTurn() == WHITE ? (limits.wtime / 20 + limits.winc / 2)
                                  : (limits.btime / 20 + limits.binc / 2);
}

} // namespace clk
