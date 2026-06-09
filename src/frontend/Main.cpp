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

#include "UCI.hpp"
#include "backend/Magic.hpp"
#include "backend/Attacks.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Hash.hpp"

/*
	IDEAS:
	1. Try to update Zobrish Key only when move is legal, in an if (legal) branch.
	2. Include node type in history bonuses
	3. Recapture heuristic (in move order try to apply capture that captures last moved piece)
	4. You've got special ZobristKey datatype instead of uint64_t.
*/

int main(int argc, const char* argv[]) {
	SlidersAttacks::initTables();
	UniversalChessInterface uci;
	uci.loop(argc, argv);
}