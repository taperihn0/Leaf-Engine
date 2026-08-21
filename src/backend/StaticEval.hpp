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

#include "Score.hpp"
#include "Tuning.hpp"
#include "Piece.hpp"

/* 
*  Static evaluation utilities.
*/

class Position;

namespace hce {

_DEFINE_TUNABLE_PARAMETER(PawnValue, int32_t, 96.1884f, 80.f, 120.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(KnightValue, int32_t, 304.315f, 260.f, 340.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(BishopValue, int32_t, 305.347f, 270.f, 340.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(RookValue, int32_t, 489.203f, 450.f, 550.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(QueenValue, int32_t, 916.466f, 820.f, 980.f, 1.1f);

_INTERNAL constexpr int16_t getPieceValue(Piece::enumType pc) {
    switch (pc) {
    case Piece::PAWN:   return PawnValue;
    case Piece::KNIGHT: return KnightValue;
    case Piece::BISHOP: return BishopValue;
    case Piece::ROOK:   return RookValue;
    case Piece::QUEEN:  return QueenValue;
    default: unreachable();
    }
}

class StaticEval {
public:
    _NODISCARD static sc::Score evaluatePawnlessEndgame(const Position& pos);
    _NODISCARD static sc::Score matEval(const Position& pos);
    _NODISCARD static sc::Score staticEval(const Position& pos);
private:
    static sc::Score pawnsStaticEval(const Position& pos, enumColor side);
    static sc::Score knightsStaticEval(const Position& pos, enumColor side);
    static sc::Score bishopsStaticEval(const Position& pos, enumColor side);
    static sc::Score rooksStaticEval(const Position& pos, enumColor side);
    static sc::Score queensStaticEval(const Position& pos, enumColor side);
    static sc::Score kingsStaticEval(const Position& pos, enumColor side);

    static std::array<int16_t, 64>
        _mg_pawn_tables, 
        _mg_knight_tables, 
        _mg_bishop_tables, 
        _mg_rook_tables, 
        _mg_queen_tables, 
        _mg_king_tables;
};

} // namespace hce
