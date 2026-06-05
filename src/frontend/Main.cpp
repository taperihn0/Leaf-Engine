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