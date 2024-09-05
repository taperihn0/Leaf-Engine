#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"

/*
	NOTES:
	(X) 1. Dynamic bit shifting with relevant bits for every single square in 
	magic hashing might be simplified to shifting by 64 - 12 bits for rooks and 
	64 - 9 bits for bishops.
	2. Implement 8x8 board representation. Hybrid approach might be faster.
	8x8 board can be used in pieceTypeOn functions. Downside of this approach would be
	a need of incrementaly updating new board.
	3. When opposite knight attack our king, the only way to get out of the check is to
	move king or capture attacking knight. How to incorporate this fact into move generator?
	4. When in double check, only king moves are possible to get out of the check.
	5. Pinned knights can't move. Check whether this idea can be practically used in move gen.
	(V) 6. Generate pre-computed static tables for knights, kings and pawns attacks for each square.
	7. Keep king square in a memory and incrementally update it. Maybe it will be faster than 
	scanning for king's square manually.

	RESULTS:
	commit [a42b979]
	 ~254 seconds, ~27220kN/sec.

	added idea 6:
	 ~222 seconds, ~31180kN/sec.
*/

StaticAttackTables attack_tables;

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}