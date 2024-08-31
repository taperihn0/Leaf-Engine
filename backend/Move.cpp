#include "Move.hpp"
#include "Position.hpp"

// TODO
Move Move::fromStr(const Position pos, const std::string str) {
#if defined(LONG_ALGEBRAIC_NOTATION)
	ASSERT(str.size() == 4 or str.size() == 5, "Invalid move");

	Move tmp;

	Square origin = Square::fromChar(str[0], str[1]),
		target = Square::fromChar(str[2], str[3]);

	tmp.setOrigin(origin), tmp.setTarget(target);

	if (str.size() == 5)
		tmp.setPromoPiece(Piece().fromChar(pos.getTurn(), str[4]));

	return tmp;
#endif
}

void Move::print() {
#if defined(LONG_ALGEBRAIC_NOTATION) 
	if (_rmove == null) {
		std::cout << _null_str;
	}
	else {
		getOrigin().print(), getTarget().print();
		if (isPromotion()) Piece(getPromoPieceT()).print();
	}
#endif
}