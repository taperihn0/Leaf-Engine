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

namespace utils {

/* BulletChessBoard is direct mapping
*  of bullet-format entry used in bullet while training.
*/
struct BulletChessBoard {
    BulletChessBoard() = default;

    // read and write functions for direct read/write from stream
    static bool write(std::ostream& output, const BulletChessBoard& bf);
    static bool read(std::istream& input, BulletChessBoard& bf);

    bool operator==(const BulletChessBoard& bf) const;
    _INLINE bool operator!=(const BulletChessBoard& bf) const { return !(*this == bf); }

    BitBoard occ;
    std::array<uint8_t, 16> 
             pcs;
    int16_t  score;
    uint8_t  result;
    uint8_t  ksq;
    uint8_t  opp_ksq;
    std::array<std::byte, 3> 
             __align;
};

/* TrainingDataEntry is like bullet-format,
*  but it stores results differently.
*  Instead of storing relative position data in respect
*  to current side to move, we just store absolute
*  position data.
*  While parsing TrainingDataEntry to the actual bullet-format
*  we need some logic for eventually fliping saved values in respect to side to move.
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

    TrainingDataEntry(const PackedPosition& packed, sc::Score white_score, Result8b result);
    TrainingDataEntry(const ExtPackedPosition& packed, sc::Score white_score, Result8b result);

    bool operator==(const TrainingDataEntry& entry) const;
    _INLINE bool operator!=(const TrainingDataEntry& entry) const { return !(*this == entry); }

    static bool write(std::ostream& output, const TrainingDataEntry& entry);
    static bool read(std::istream& input, TrainingDataEntry& entry);
    static BulletChessBoard toBulletFormat(const TrainingDataEntry& entry);

    const PackedPosition& getPosition() const;
    sc::Score getWhiteScore() const;
    Result8b getGameResult() const;
private:
#pragma pack(push, 1)
    struct PackedPosInfo {
        PackedPosInfo() = default;

        bool operator==(const PackedPosInfo& info) const;
        _INLINE bool operator!=(const PackedPosInfo& info) const { return !(*this == info); }

        sc::Score             white_score;
        Result8b              result;
        Square                king_sq;
        Square                opp_king_sq;
        // Bulletformat use relative position data, but 
        // current training data entry uses absolute position data.
        // Training data that was generated before adding this mark
        // might have this flag set to 'true'.
        bool                  relative = false;
        std::array<std::byte, 2> __align;
    };
#pragma pack(pop)

    static_assert(sizeof(PackedPosInfo) == 8);

    PackedPosition _packed_pos;
    PackedPosInfo  _game_details;
};

} // namespace utils
