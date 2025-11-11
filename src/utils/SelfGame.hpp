#pragma once

#include "backend/Game.hpp"
#include "backend/Search.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace Utils {

class SelfGame {
public:
    SelfGame(size_t tt_size = 1_MB);

    Game::Result start(std::vector<PackedPosition>& positions, SearchLimits limits);
private:
    static inline const std::string _OpeningFile = "src/assets/openingsPositions/crafty_2500_new.epd";

    Search _search_by_side[2];
    static OpeningGenerator _openings;
};

} // namespace Utils
