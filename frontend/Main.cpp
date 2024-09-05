#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"

/*
	NOTES:
	2. Implement 8x8 board representation. Hybrid approach might be faster.
	8x8 board can be used in pieceTypeOn functions. Downside of this approach would be
	a need of incrementaly updating new board.
	3. When opposite knight attack our king, the only way to get out of the check is to
	move king or capture attacking knight. How to incorporate this fact into move generator?
*/

StaticAttackTables attack_tables;

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}