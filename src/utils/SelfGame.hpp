#pragma once

#include "UtilsCommon.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace Utils {

class SelfGame {
public:
    SelfGame(size_t tt_size_per_search = 1_MB);

    Game::Result start(SearchLimits limits);
    Game::Result start(std::vector<ExtPackedPosition>& packed_positions, SearchLimits limits);
private:
    template <bool CollectData>
    Game::Result setupMatch(SearchLimits limits, std::vector<ExtPackedPosition>* const packed_positions);

    Search _search_by_side[2];
};

} // namespace Utils
