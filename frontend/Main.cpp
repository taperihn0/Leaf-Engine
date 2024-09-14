#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"
#include "../backend/MoveGen.hpp"

/*
	NOTES:
	1. Not always return draw score. Sometimes it may be beneficial
	to return different score, so called contempt factor. An idea
	here would be to register approximate strenght of an opponent and 
	basing on that calculate appopriate draw score
	2. Try to update Zobrish Key only when move is legal, in an if (legal) branch.
	3. Also, try another depth when considering moves from game record in terms of repetitions.
	4. When draw is found and it increases alpha, PV might be invalid because of overwriting old
	paths from previous moves.
	5. Game state of a node don't have to be overwritten every time we make a move. It can be 
	remembered once, when entering the node.
*/

const StaticAttackTables attack_tables;
const RectangularTable rectangular;

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();
	
	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}