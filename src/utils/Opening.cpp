#include "Opening.hpp"
#include "backend/MoveGen.hpp"

namespace Utils {

OpeningGenerator::OpeningGenerator(std::string epd_openings)
: _openings_file_path(epd_openings) {}

void OpeningGenerator::getFilePositions() {
    if (!_positions.empty())
        return;
    
    _openings_file.open(_openings_file_path);

    if (!_openings_file) {
        ASSERT(false, "Failed to open " + _openings_file_path);
        return;
    }

    std::string line;

    while (std::getline(_openings_file, line)) {
        Position pos(line);
        _positions.push_back(std::move(pos));
    }

    std::random_shuffle(_positions.begin(), _positions.end());

    _openings_file.close();
}

Position OpeningGenerator::get() {
    ASSERT(!_positions.empty(), "Empty position buffer");
    size_t random_index = random<size_t>(0, _positions.size() - 1);
    Position position = _positions.at(random_index);
    return randomizePosition(position);
}

Position OpeningGenerator::randomizePosition(Position& pos) {
    for (int i = 0; i < MinRandomMoves; i++) {
        Move32b random_move = MoveGen::generateRandomMove<MoveGen::ALL>(pos);
        Position::IrreversibleState state = pos.getIrreversibleState();

        if (!pos.make(random_move))
            pos.unmake(random_move, state);
    }

    return pos;
}

} // namespace Utils
