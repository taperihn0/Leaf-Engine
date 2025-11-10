#include "UCI.hpp"
#include "backend/Magic.hpp"
#include "backend/Attacks.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Hash.hpp"

/*
	NOTES:
	1. Not always return Draw score. Sometimes it may be beneficial
	to return different score, so called contempt factor. An idea
	here would be to register approximate strenght of an opponent and 
	basing on that calculate appopriate Draw score
	2. Try to update Zobrish Key only when move is legal, in an if (legal) branch.
	3. Also, try another depth when considering moves from game record in terms of repetitions.
	4. Try to score captures only once (faster) instead of doing that every time a new capture is picked
	5. Try different method of updating history tables for quiet moves (also, try different aging)
	6. Recapture heuristic (in move order try to apply capture that captures last moved piece)
	7. Experiment with verification search in Null Move32b Pruning
	8. Restrict Late Move32b Reduction
*/

int main(int argc, const char* argv[]) {
	ZobristHash::fillKeys();
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}