#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"
#include "../backend/MoveGen.hpp"

const StaticAttackTables attack_tables;
const RectangularTable rectangular;

/*
	NOTES:
	1. Not always return draw score. Sometimes it may be beneficial
	to return little different score, so called contempt factor. An idea
	here would be to register approximate strenght of an opponent and 
	based on that calculate appopriate draw score.
	2. Try to update Zobrish Key only when move is legal, in an if (legal) branch.
	3. Try to find more efficient algorithm behing cycle detection (repetition) in search.
*/

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();
	
	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}