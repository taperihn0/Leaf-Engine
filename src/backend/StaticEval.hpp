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
#include "Color.hpp"

/* 
*  Static evaluation utilities.
*/

class Position;

_PARAM_ATTRIBS int PawnValue   = 100;
_PARAM_ATTRIBS int KnightValue = 300;
_PARAM_ATTRIBS int BishopValue = 300;
_PARAM_ATTRIBS int RookValue   = 500;
_PARAM_ATTRIBS int QueenValue  = 900;

inline array1d<const int*, 5> PieceValue = {
    reinterpret_cast<const int*>(&PawnValue), 
    reinterpret_cast<const int*>(&KnightValue), 
    reinterpret_cast<const int*>(&BishopValue), 
    reinterpret_cast<const int*>(&RookValue), 
    reinterpret_cast<const int*>(&QueenValue),
};

class StaticEval {
public:
    _NODISCARD static Score evaluateEndgame(const Position& pos);
    _NODISCARD static Score matEval(const Position& pos);
    _NODISCARD static Score staticEval(const Position& pos);
private:
    static Score pawnsStaticEval(const Position& pos, enumColor side);
    static Score knightsStaticEval(const Position& pos, enumColor side);
    static Score bishopsStaticEval(const Position& pos, enumColor side);
    static Score rooksStaticEval(const Position& pos, enumColor side);
    static Score queensStaticEval(const Position& pos, enumColor side);
    static Score kingsStaticEval(const Position& pos, enumColor side);

    static array1d<int16_t, 64>
        _mg_pawn_tables, 
        _mg_knight_tables, 
        _mg_bishop_tables, 
        _mg_rook_tables, 
        _mg_queen_tables, 
        _mg_king_tables;
};
