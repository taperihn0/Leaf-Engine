#include "Common.hpp"
#include "BitBoard.hpp"

class Position {
public:
	enum enumPiece : uint8_t {
		PAWN,
		KNIGHT,
		BISHOP,
		ROOK,
		QUEEN,
		KING
	};

	enum enumColor : bool {
		WHITE, BLACK
	};

	void setByFEN(const std::string fen);

	INLINE BitBoard getPawnsBySide(enumColor col_type) {
		return _piece_bb[col_type][PAWN];
	}

	INLINE BitBoard getKnightsBySide(enumColor col_type) {
		return _piece_bb[col_type][KNIGHT];
	}

	INLINE BitBoard getBishopsBySide(enumColor col_type) {
		return _piece_bb[col_type][BISHOP];
	}

	INLINE BitBoard getRooksBySide(enumColor col_type) {
		return _piece_bb[col_type][ROOK];
	}

	INLINE BitBoard getQueensBySide(enumColor col_type) {
		return _piece_bb[col_type][QUEEN];
	}

	INLINE BitBoard getKingBySide(enumColor col_type) {
		return _piece_bb[col_type][KING];
	}

	INLINE BitBoard getWhites() {
		return _piece_bb[WHITE][PAWN]
			| _piece_bb[WHITE][KNIGHT]
			| _piece_bb[WHITE][BISHOP]
			| _piece_bb[WHITE][ROOK]
			| _piece_bb[WHITE][QUEEN]
			| _piece_bb[WHITE][KING];
	}

private:
	std::array<std::array<BitBoard, 6>, 2> _piece_bb;
	enumColor turn;
};