#include "Opening.hpp"
#include "backend/StaticEval.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Score.hpp"

namespace Utils {

std::vector<Position> OpeningGenerator::_positions;

void OpeningGenerator::load() {
    if (!_positions.empty())
        return;
    
    std::ifstream openings_file(_OpeningFile);

    if (!openings_file) {
        ASSERT(false, "Failed to open " + _OpeningFile);
        return;
    }

    std::cout << "Loading openings positions..." << std::endl;
    
    std::string line;

    while (std::getline(openings_file, line)) {
        Position pos_from_fen(line);

        _positions.push_back(pos_from_fen);

        for (int i = 0; i < _RandomPerPos; i++) {
            Position pos = pos_from_fen;

            for (int j = 1; j <= _MaxRandomMoves; j++) {
                Move32b random_move = MoveGen::getRandomLegalMove<MoveGen::ALL>(pos);

                if (random_move.isNull())
                    break;

                bool legal = pos.make(random_move);
                assert(legal);

                if (j >= _MinRandomMoves)
                    _positions.push_back(pos);
            }
        }
    }

    auto last = std::unique(_positions.begin(), _positions.end());

    last = std::remove_if(_positions.begin(), last, [](Position& pos) {
        return std::abs(static_cast<int>(StaticEval::staticEval(pos))) > _OpeningEvalThreshold;
    });

    _positions.erase(last, _positions.end());

    std::shuffle(_positions.begin(), _positions.end(), GlobMersenne);

    std::cout << "Successfully loaded " << _positions.size() << " opening positions" << std::endl;
}

const Position& OpeningGenerator::getPosition() {
    if (_positions.empty()) {
        ASSERT(false, "Openings are not loaded");
    }

    size_t random_index = random<size_t>(0, _positions.size() - 1);
    return _positions[random_index];
}

OpeningSuite::OpeningSuite(std::string path) {
    load(path);
}

void OpeningSuite::load(std::string path) {
    std::ifstream openings_file(path);

    if (!openings_file) {
        ASSERT(false, "Failed to open " + path);
        return;
    }

    std::string line;

    while (getline(openings_file, line)) {
        Position pos_from_line(line);
        _positions.push_back(pos_from_line);
    }
}

const Position& OpeningSuite::getRandomPosition() {
    if (_positions.empty()) {
        ASSERT(false, "Openings are not loaded");
    }

    size_t random_index = random<size_t>(0, _positions.size() - 1);
    return _positions[random_index];
}

bool OpeningSuite::isEmpty() const
{
    return _positions.empty();
}

} // namespace Utils
