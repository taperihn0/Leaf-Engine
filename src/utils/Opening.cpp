#include "Opening.hpp"
#include "backend/Eval.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Score.hpp"

namespace Utils {

OpeningGenerator::OpeningGenerator(std::string epd_openings)
: _openings_file_path(epd_openings) {}

void OpeningGenerator::load() {
    if (!_positions.empty())
        return;
    
    _openings_file.open(_openings_file_path);

    if (!_openings_file) {
        ASSERT(false, "Failed to open " + _openings_file_path);
        return;
    }

    std::cout << "Loading openings positions..." << std::endl;
    
    std::string line;

    while (std::getline(_openings_file, line)) {
        Position pos_from_fen(line);
        
        if (std::abs(Eval::staticEval(pos_from_fen).toInt()) <= _OpeningEvalThreshold
            and pos_from_fen.halfmoveClock() < 10) {

            _positions.push_back(std::move(pos_from_fen));

            for (int i = 0; i < _RandomPerPos; i++) {
                Position pos = pos_from_fen;

                for (int j = 0; j < _MaxRandomMoves; j++) {
                    Move32b random_move = MoveGen::generateRandomMove<MoveGen::ALL>(pos);
                    Position::IrreversibleState state = pos.getIrreversibleState();

                    if (!pos.make(random_move)) {
                        pos.unmake(random_move, state);
                        continue;
                    }

                    if (j >= _MinRandomMoves)
                        _positions.push_back(pos);
                }

                _positions.push_back(std::move(pos));
            }
        }
    }

    auto last = std::unique(_positions.begin(), _positions.end());
    _positions.erase(last, _positions.end());

    std::mt19937 mersenne(1);
    std::shuffle(_positions.begin(), _positions.end(), mersenne);
    
    _openings_file.close();

    std::cout << "Successfully loaded " << _positions.size() << " opening positions" << std::endl;
}

Position OpeningGenerator::getPosition() {
    ASSERT(!_positions.empty(), "Empty position buffer");
    size_t random_index = random<size_t>(0, _positions.size() - 1);
    return _positions[random_index];
}

} // namespace Utils
