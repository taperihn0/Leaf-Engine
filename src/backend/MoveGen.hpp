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

#include "Position.hpp"
#include "MoveList.hpp"

class MoveGen {
public:

    enum enumGenMoves : uint8_t {
        CAPTURES                   = 1,
        NON_CAPTURES               = 1 << 1,
        QUEENPROMOS                = 1 << 2,
        UNDERPROMOS                = 1 << 3,
        TACTICALS_ONLY_QUEENPROMOS = CAPTURES | QUEENPROMOS,
        TACTICALS_ALL_PROMOS       = CAPTURES | QUEENPROMOS | UNDERPROMOS,
        QUIETS_NO_PROMOS           = NON_CAPTURES,
        QUIETS_ONLY_UNDERPROMOS    = NON_CAPTURES | UNDERPROMOS,
        ALL                        = CAPTURES | NON_CAPTURES | QUEENPROMOS | UNDERPROMOS
    };

    template <enumGenMoves Moves2Gen>
    static void generatePseudoLegalMoves(const Position& pos, ml::MoveList& move_list);

    template <enumGenMoves Moves2Gen>
    static void generateLegalMoves(Position& pos, ml::MoveList& move_list);

    template <enumGenMoves Moves2Gen>
    _NODISCARD static Move32b getRandomLegalMove(Position& pos);

    _NODISCARD static bool isAnyCapture(Position& pos);
};

constexpr MoveGen::enumGenMoves operator|(MoveGen::enumGenMoves node0, MoveGen::enumGenMoves node1) {
    return static_cast<MoveGen::enumGenMoves>(static_cast<int>(node0) | static_cast<int>(node1));
}

