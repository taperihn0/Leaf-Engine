#include "UCI.hpp"
#include "../backend/Magic.hpp"

int main(int argc, const char* argv[]) {
	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);

	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	for (Square sq : { Square::a1, Square::a8, Square::e4, Square::d4, Square::f5, Square::h8, Square::f7 }) {
		sq.print();
		std::cout << '\n';

		SlidersMagics::rookAttacks(sq, Position(Position::empty_fen).getOccupied()).printRaw();

		std::cout << '\n';
	}
}