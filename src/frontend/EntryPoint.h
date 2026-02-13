#include "UCI.hpp"
#include "backend/Magic.hpp"
#include "backend/Attacks.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Hash.hpp"

_INTERNAL void startEngine(int argc, const char* argv[]) {
    ZobristHash::fillKeys();
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}
