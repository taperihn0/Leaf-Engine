#pragma once

#include "backend/Common.hpp"
#include "backend/Position.hpp"

#include <vector>
#include <fstream>

namespace Utils {

/* OpeningGenerator takes care of openings set.
*  Usually, these are too small to be just used while setting up 
*  very long tournamets, so we need to add some noise to the existing positions.
*  See randomizePosition for more details.
*/
class OpeningGenerator {
public:
    OpeningGenerator(std::string epd_openings);

    void getFilePositions();

    Position get();
private:

    /* Avaible opening sets are usually really small and compact, 
    *  containing at most few thousands positions.
    *  Here, we randomize some random position from set by applying 
    *  few more random moves.
    */
    Position randomizePosition(Position& pos);

    static constexpr int MinRandomMoves = 6;

    std::vector<Position> _positions;
    std::string _openings_file_path;
    std::ifstream _openings_file;
};

} // namespace Utils
