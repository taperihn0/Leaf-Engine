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

#include "backend/Common.hpp"
#include "backend/Position.hpp"
#include "backend/Move.hpp"
#include "backend/Time.hpp"
#include "backend/Search.hpp"
#include "backend/Game.hpp"

#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <functional>
#include <mutex>

#if defined(DEBUG)
#define INSPECT_SELFPLAY_MATCHES
#endif

namespace utils {

_INLINE size_t getIStreamBytesLeft(std::istream& input) {
    auto curr_bytes = input.tellg();    
    input.seekg(0, std::ios::end);
    auto end_bytes = input.tellg();
    input.seekg(curr_bytes);
    return end_bytes - curr_bytes;
}

template <typename Return, typename... Args>
Return doNothing(Args&&...) { return Return(); };

static int PlatformThreadLimit = []() -> int {
    return std::thread::hardware_concurrency();
}();

inline std::string_view ProcExecArg = "<empty>";

_FORCEINLINE std::istream& readline(std::istream& os, std::string& line) {
    return std::getline(os, line);
}

static constexpr int SelfPlaySessionCountLimit = 256;

} // namespace utils
