#pragma once

#include "backend/Common.hpp"
#include "backend/Position.hpp"

#include <vector>
#include <fstream>

namespace Utils {

/* OpeningGenerator takes care of openings set.
*  Usually, these are too small to be just used while setting up 
*  very long tournamets, so we need to add some noise to the existing positions.
*/
class OpeningGenerator {
public:
    static void load();

    static const Position& getPosition();
private:
    static inline const std::string _OpeningFile = "src/assets/openingsPositions/crafty_2500_new.epd";

    static constexpr int            _OpeningEvalThreshold = 400;
    static constexpr int            _RandomPerPos   = 25;
    static constexpr int            _MinRandomMoves = 2;
    static constexpr int            _MaxRandomMoves = 14;

    static std::vector<Position>    _positions;
    static std::ifstream            _openings_file;
};

} // namespace Utils
