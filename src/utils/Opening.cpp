#include "Opening.hpp"
#include "backend/StaticEval.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Score.hpp"
#include "NetworkEval.hpp"

namespace Utils {

std::vector<OpeningGenerator::GeneratedPosition> OpeningGenerator::_positions;

bool OpeningGenerator::GeneratedPosition::operator==(const OpeningGenerator::GeneratedPosition& p) {
    return pos == p.pos;
}

bool OpeningGenerator::GeneratedPosition::operator!=(const OpeningGenerator::GeneratedPosition& p) {
    return !(*this == p);
}

OpeningGenerator OpeningGenerator::create() {
    static bool created = false;

    if (created) {
        ASSERT(false, "Singleton object already created");
    }

    return OpeningGenerator();
}

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

        _positions.push_back(GeneratedPosition{ 0, pos_from_fen });

        for (int i = 0; i < _RandomPerPos; i++) {
            Position pos = pos_from_fen;

            for (int j = 1; j <= _MaxRandomMoves; j++) {
                Move32b random_move = MoveGen::getRandomLegalMove<MoveGen::ALL>(pos);

                if (random_move.isNull())
                    break;

                bool legal = pos.make(random_move);
                assert(legal);

                if (j >= _MinRandomMoves)
                    _positions.push_back(GeneratedPosition{ j, pos });
            }
        }
    }

    auto last = std::unique(_positions.begin(), _positions.end());

    last = std::remove_if(_positions.begin(), last, [](GeneratedPosition& genpos) {
        const Score score = nn::NEval::evaluate(nn::GlobPackedNetwork, genpos.pos);

        if (std::abs(static_cast<int>(score)) > _OpeningEvalThreshold)
            return true;

        else if (genpos.pos.isInCheck(genpos.pos.getTurn()))
            return true;

        MoveList ml;
        MoveGen::generateLegalMoves<MoveGen::ALL>(genpos.pos, ml);

        if (!ml.count())
            return true;

        return false;
    });

    _positions.erase(last, _positions.end());

    std::shuffle(_positions.begin(), _positions.end(), GlobMersenne);

    std::cout << "Successfully loaded " << _positions.size() << " opening positions" << std::endl;
}

const Position& OpeningGenerator::getRandomPosition(int& moves_done) const {
    if (_positions.empty()) {
        ASSERT(false, "Openings are not loaded");
    }

    size_t random_index = random<size_t>(0, _positions.size() - 1);
    moves_done = _positions[random_index].moves_done;
    return _positions[random_index].pos;
}

bool OpeningGenerator::isEmpty() const {
    return _positions.empty();
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

const Position& OpeningSuite::getRandomPosition(int& moves_done) const {
    if (_positions.empty()) {
        ASSERT(false, "Openings are not loaded");
    }

    size_t random_index = random<size_t>(0, _positions.size() - 1);
    moves_done = 0;
    return _positions[random_index];
}

bool OpeningSuite::isEmpty() const {
    return _positions.empty();
}

} // namespace Utils
