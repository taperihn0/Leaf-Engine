#include "UCI.hpp"
#include "../backend/Attacks.hpp"

int main(int argc, const char* argv[]) {
	UniversalChessInterface uci_obj;
	//uci_obj.loop(argc, argv);

	for (Square sq : { Square::a1, Square::a8, Square::f5, Square::c4, Square::f7 }) {
		sq.print();
		std::cout << '\n';

		rayAttacksQueen(sq).printRaw();

		std::cout << '\n';
	}
}