#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"

/*
	NOTES:
	1. Implement 8x8 board representation. Hybrid approach might be faster.
	8x8 board can be used in pieceTypeOn functions. Downside of this approach would be
	a need of incrementaly updating new board.

	CURRENT RESULTS for position (depth 6):
	[r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10]
	-> (217.774 seconds, 31790kN/sec.)
	changed if-branches in make function:
	-> (204.242 seconds, 33896kN/sec.)
	delegated variable definitions in make and unmake
	-> (192.031 seconds, 36051kN/sec.)
	changed if-branch for captures and r-value references in MoveList::push
	-> (190.25 seconds, 36389kN/sec.)
*/

const StaticAttackTables attack_tables;
const RectangularTable rectangular;

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}