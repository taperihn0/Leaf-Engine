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

namespace Utils {

_INLINE size_t streamBytesLeft(std::istream& input) {
    auto curr_bytes = input.tellg();
    input.seekg(0, std::ios::end);
    auto end_bytes = input.tellg();
    input.seekg(curr_bytes);
    return end_bytes - curr_bytes;
}

} // namespace Utils
