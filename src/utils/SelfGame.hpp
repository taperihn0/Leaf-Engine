#pragma once

#include "backend/Game.hpp"
#include "backend/Search.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace Utils {

class SelfGame {
public:
    SelfGame(size_t tt_size = 1_MB);
    Game::Result start(std::vector<PackedPosition>& packed_positions, SearchLimits limits);
private:
    Search _search_by_side[2];
};

} // namespace Utils
