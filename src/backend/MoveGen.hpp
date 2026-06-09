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

class MoveGenerator {
public:

	/*
	*	Generation mode:
	*	<CAPTURES> - all pseudo-legal captures and queen promotions
	*	<TACTICALS> - all pseudo-legal captures and all promotions
	*	<QUIETS> - all pseudo-legal non-captures and promotions without queen promotions
	*	<ALL> - all pseudo-legal moves in given position
	*/

	enum enumMode : uint8_t {
		CAPTURES,
		TACTICALS,
		QUIETS,
		ALL,
	};

	template <enumMode GenType>
	static void generatePseudoLegalMoves(const Position& pos, MoveList& move_list);

    template <enumMode GenType>
    static void generateLegalMoves(Position& pos, MoveList& move_list);

	template <enumMode GenType>
	_NODISCARD static Move32b getRandomLegalMove(Position& pos);

	_NODISCARD static bool isAnyCapture(Position& pos);
};

// helpful alias for MoveGenerator class
using MoveGen = MoveGenerator;
