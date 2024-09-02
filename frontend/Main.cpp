#include "UCI.hpp"
#include "../backend/Magic.hpp"

/*
	NOTES:
	1. Dynamic bit shifting with relevant bits for every single square in 
	magic hashing might be simplified to shifting by 64 - 12 bits for rooks and 
	64 - 9 bits for bishops. 
	2. Implement 8x8 board representation. Hybrid approach might be faster.
	8x8 board can be used in pieceTypeOn functions. Downside of this approach would be
	a need of incrementaly updating new board.
	3. When opposite knight attack our king, the only way to get out of the check is to
	move king or capture attacking knight. How to incorporate this fact into move generator?
	4. When in double check, only king moves are possible to get out of the check.
	5. Pinned knights can't move. Check whether this idea can be practically used in move gen.
*/

int main(int argc, const char* argv[]) {
	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}