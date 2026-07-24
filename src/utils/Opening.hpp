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

#pragma once

#include "backend/Common.hpp"
#include "backend/Position.hpp"

#include <vector>
#include <fstream>

#if !defined(_CRAFTY_OPENING_SET) and \
    !defined(_UHO_OPENING_SET)
#define _CRAFTY_OPENING_SET
#endif

namespace utils {

class OpeningManBase
{
public:
    virtual const Position& getRandomPosition(int& moves_done) const = 0;
    virtual bool isEmpty() const = 0;
};

/* OpeningGenerator takes care of openings set.
*  Usually, these are too small to be just used while setting up 
*  very long tournamets, so we need to add some noise to the existing positions.
*/
class OpeningGenerator : public OpeningManBase {
public:
    static OpeningGenerator create();
    void load();
    const Position& getRandomPosition(int& moves_done) const override;
    bool isEmpty() const override;
private:
    OpeningGenerator() = default;

    static constexpr int _OpeningEvalThreshold = 300;
#if !defined(DEBUG)
    static constexpr int _RandomPerPos   = 35;
#else
    static constexpr int _RandomPerPos   = 2;
#endif
    static constexpr int _MinRandomMoves = 2;
    static constexpr int _MaxRandomMoves = 10;

    struct GeneratedPosition {
        int moves_done;
        Position pos;

        bool operator==(const GeneratedPosition& p);
        bool operator!=(const GeneratedPosition& p);
    };

    static std::vector<GeneratedPosition> _positions;
};

inline OpeningGenerator GlobOpeningGenerator = []() -> OpeningGenerator {
    return OpeningGenerator::create();
}();

/* Regular EPD Openings loader.
*  Setups very little opening positions.
*/
class OpeningSuite : public OpeningManBase {
public:
    OpeningSuite() = default;
    OpeningSuite(const std::filesystem::path& path);

    void loadFromFile(const std::filesystem::path& path);

    template <typename Str, typename = std::enable_if_t<
        std::is_same_v<Str, std::string_view> or std::is_same_v<Str, std::string>>
    >
    void loadFromVec(const std::vector<Str>& fens);
    const Position& getRandomPosition(int& moves_done) const override;
    bool isEmpty() const override;
private:
    std::vector<Position> _positions;
};

} // namespace utils
