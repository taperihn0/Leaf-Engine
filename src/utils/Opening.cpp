/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Opening.hpp"
#include "backend/StaticEval.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Score.hpp"
#include "NetworkEval.hpp"
#include "Sets.hpp"

namespace utils {

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
        FAILED("Singleton object already created");
    }

    return OpeningGenerator();
}

void OpeningGenerator::load() {
    if (!_positions.empty())
        return;

    std::cout << "Loading openings positions..." << std::endl;

#if defined(_UHO_OPENING_SET)
    const std::vector<std::string>& openingset = getLichessUHO_Openings();
#else
    const std::vector<std::string>& openingset = getCraftyOpenings();
#endif

    std::for_each(openingset.begin(), openingset.end(), [&](const std::string& fen) {
        const Position pos_from_fen(fen);

        _positions.push_back(GeneratedPosition{ 0, pos_from_fen });

        for (int i = 0; i < _RandomPerPos; i++) {
            Position pos = pos_from_fen;

            for (int j = 1; j <= _MaxRandomMoves; j++) {
                Move32b random_move = MoveGen::getRandomLegalMove<MoveGen::ALL>(pos);

                if (random_move.isNullMove())
                    break;

                const bool legal = pos.make(random_move);
                assert(legal);

                if (j >= _MinRandomMoves)
                    _positions.push_back(GeneratedPosition{ j, pos });
            }
        }
    });

    auto last = std::unique(_positions.begin(), _positions.end());

    last = std::remove_if(_positions.begin(), last, [](GeneratedPosition& genpos) {
        const sc::Score score = nn::NEval::evaluate(nn::GlobPackedNetwork, genpos.pos);

        if (std::abs(static_cast<int>(score)) > _OpeningEvalThreshold)
            return true;

        else if (genpos.pos.isInCheck(genpos.pos.getTurn()))
            return true;

        ml::MoveList ml;
        MoveGen::generateLegalMoves<MoveGen::ALL>(genpos.pos, ml);

        if (!ml.count())
            return true;

        return false;
    });

    _positions.erase(last, _positions.end());
    std::shuffle(_positions.begin(), _positions.end(), rnd::GlobMersenne);

    std::cout << "Successfully loaded " << _positions.size() << " opening positions" << std::endl;
}

const Position& OpeningGenerator::getRandomPosition(int& moves_done) const {
    if (_positions.empty()) {
        FAILED("Openings are not loaded");
    }

    const size_t random_index = rnd::random<size_t>(0, _positions.size() - 1);
    moves_done = _positions[random_index].moves_done;
    return _positions[random_index].pos;
}

bool OpeningGenerator::isEmpty() const {
    return _positions.empty();
}

OpeningSuite::OpeningSuite(const std::filesystem::path& path) {
    loadFromFile(path);
}

void OpeningSuite::loadFromFile(const std::filesystem::path& path) {
    std::ifstream openings_file(path);

    if (!openings_file) {
        FAILED("Failed to open " + path.string());
        return;
    }

    for (std::string line; std::getline(openings_file, line); ) {
        const Position pos_from_line(line);
        _positions.push_back(std::move(pos_from_line));
    }
}

template <typename Str, typename /* = std::enable_if_t<
    std::is_same_v<T, std::string_view> or std::is_same_v<T, std::string>> */
>
void OpeningSuite::loadFromVec(const std::vector<Str>& fens) {
    std::for_each(fens.begin(), fens.end(), [&](const Str& fen) {
        const Position pos(fen);
        _positions.push_back(std::move(pos));
    });
}

const Position& OpeningSuite::getRandomPosition(int& moves_done) const {
    if (_positions.empty()) {
        FAILED("Openings are not loaded");
    }

    size_t random_index = rnd::random<size_t>(0, _positions.size() - 1);
    moves_done = 0;
    return _positions[random_index];
}

bool OpeningSuite::isEmpty() const {
    return _positions.empty();
}

template void OpeningSuite::loadFromVec<std::string>(const std::vector<std::string>& fens);
template void OpeningSuite::loadFromVec<std::string_view>(const std::vector<std::string_view>& fens);

} // namespace utils
