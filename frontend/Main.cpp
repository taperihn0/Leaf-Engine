#include "UCI.hpp"
#include "../backend/Magic.hpp"
#include "../backend/Attacks.hpp"
#include "../backend/MoveGen.hpp"

const StaticAttackTables attack_tables;
const RectangularTable rectangular;

int main(int argc, const char* argv[]) {
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}