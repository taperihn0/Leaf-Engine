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

#include "PackedPosition.hpp"

namespace Utils {

/* TrainingDataEntry is like bullet-format,
*  but it stores results differently.
*  Instead of storing relative results in respect
*  to current side to move, we just store absolute
*  result of the game.
*  While parsing TrainingDataEntry to the actual bullet-format
*  we need some logic for eventually fliping result value.
*  To do that, we need to get information about current side to move
*  by checking which king is placed in 'TrainingDataEntry::PackedPosInfo::king_sq' square.
*/
class TrainingDataEntry {
public:
    TrainingDataEntry() = default;

    // Absolute game result.
    enum Result8b : uint8_t {
        BLACK_WIN = 0,
        DRAW      = 1,
        WHITE_WIN = 2,
    };

    TrainingDataEntry(const PackedPosition& packed, Score white_score, Result8b result);
    TrainingDataEntry(const ExtPackedPosition& packed, Score white_score, Result8b result);

    static bool write(std::ostream& output, const TrainingDataEntry& entry);
    static bool read(std::istream& input, TrainingDataEntry& entry);

    PackedPosition getPosition() const;
    Score          getWhiteScore() const;
    Result8b       getGameResult() const;
private:
#pragma pack(push, 1)
    struct PackedPosInfo {
        Score                 white_score;
        Result8b              result;
        Square                king_sq;
        Square                opp_king_sq;
        // Bulletformat use relative game results, but 
        // current training data entry uses absolute game results.
        // Training data that was generated before adding this mark
        // might have this flag set to 'true'.
        bool                  relative_result = false;
        array1d<std::byte, 2> __align;
    };
#pragma pack(pop)

    static_assert(sizeof(PackedPosInfo) == 8);

    PackedPosition _packed_pos;
    PackedPosInfo  _game_details;
};

} // namespace Utils
