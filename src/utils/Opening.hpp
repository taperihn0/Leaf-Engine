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
    OpeningGenerator(std::string epd_openings);

    void load();

    Position getPosition();
private:
    static constexpr int _OpeningEvalThreshold = 300;
    static constexpr int _RandomPerPos   = 10;
    static constexpr int _MinRandomMoves = 2;
    static constexpr int _MaxRandomMoves = 10;

    std::vector<Position> _positions;
    std::string _openings_file_path;
    std::ifstream _openings_file;
};

} // namespace Utils
