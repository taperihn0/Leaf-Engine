#pragma once

#include "BitBoard.hpp"
#include "Piece.hpp"
#include "Attacks.hpp"

class Move;
class Position;

class Turn {
public:
	Turn() = default;
	INLINE constexpr Turn(enumColor c)
		: _col(c) {}

	INLINE constexpr operator enumColor() const {
		return _col;
	}

	INLINE constexpr Turn operator=(enumColor c) {
		return _col = c;
	}

	INLINE constexpr Turn operator!() const {
		return !_col;
	}

	void fromChar(char c) {
		_col = c == 'w' ? WHITE : BLACK;
	}

	void print() {
		std::cout << (_col == WHITE ? 'w' : 'b');
	}

private:
	enumColor _col;
};

// wrapper around castling rights for single player
class CastlingRights {
public:
	CastlingRights() = default;
	CastlingRights(bool kinit, bool qinit);

	void printByColor(enumColor col_type);

	INLINE bool isShortPossible() const {
		return _kingside;
	}

	INLINE bool isLongPossible() const {
		return _queenside;
	}

	template <enumColor Side>
	bool notThroughCheck_Short(const Position& pos) const;

	template <enumColor Side>
	bool notThroughCheck_Long(const Position& pos) const;

	template <enumColor Side>
	bool notThroughPieces_Short(const Position& pos) const;

	template <enumColor Side>
	bool notThroughPieces_Long(const Position& pos) const;

	INLINE void setKingSide(bool flag) {
		_kingside = flag;
	}

	INLINE void setQueenSide(bool flag) {
		_queenside = flag;
	}

	INLINE void clear() {
		_kingside = false, _queenside = false;
	}
private:
	bool _kingside, _queenside;
};

// internal board state, including piece distribution 
// and game flags like castling
class Position {
public:
	Position();
	Position(const std::string init_fen);
	Position(const std::string_view init_fen);

	// assuming given FEN is valid FEN position
	void setByFEN(const std::string fen);
	void setStartingPos();

	void print() const;

	INLINE BitBoard getPawnsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::PAWN];
	}

	INLINE BitBoard getKnightsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::KNIGHT];
	}

	INLINE BitBoard getBishopsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::BISHOP];
	}

	INLINE BitBoard getRooksBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::ROOK];
	}

	INLINE BitBoard getQueensBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::QUEEN];
	}

	INLINE BitBoard getBishopsQueens(enumColor col_type) const {
		return getBishopsBySide(col_type) | getQueensBySide(col_type);
	}

	INLINE BitBoard getRooksQueens(enumColor col_type) const {
		return getRooksBySide(col_type) | getQueensBySide(col_type);
	}

	INLINE BitBoard getKingBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::KING];
	}

	BitBoard getByColor(enumColor col_type) const;

	INLINE BitBoard getWhites() const {
		return getByColor(WHITE);
	}

	INLINE BitBoard getBlacks() const {
		return getByColor(BLACK);
	}

	INLINE BitBoard getOccupied() const {
		return getWhites() | getBlacks();
	}

	INLINE BitBoard getOppositePieces() const {
		return getByColor(!_turn);
	}

	INLINE BitBoard getEmpties() const {
		return getOccupied() ^ BitBoard::universe;
	}

	INLINE Turn getTurn() const {
		return _turn;
	}

	INLINE Turn getOppositeTurn() const {
		return !_turn;
	}

	INLINE Square getEnPassantSq() const {
		return _ep_square;
	}

	template <Piece::enumType Piece, enumColor Color>
	BitBoard get() const;

	INLINE CastlingRights getCastlingByColor(enumColor col_type) const {
		return _castling_rights[col_type];
	}

	INLINE void setTurn(enumColor col_to_move) {
		_turn = col_to_move;
	}

	// returns true whether square is attacked by any enemy piece excluding enemy king
	bool attacked(Square sq) const;

	// just like attacked function above, but includes king attacks
	bool attacked_KingIncluded(Square sq) const;

	bool isCheck() const;

	void make(Move move);
	void unmake(Move move);

	template <bool Root>
	uint64_t perft(unsigned depth);

	static constexpr std::string_view starting_fen 
		= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	static constexpr std::string_view empty_fen
		= "/////// w - - 0 1";

private:
	void clearPieces();

	void setGameStatesFromStr(const std::string fen, int i);

	Piece::enumType pieceTypeOn(Square sq, enumColor by_color) const;
	Piece::enumType pieceTypeOn(Square sq) const;

	std::array<std::array<BitBoard, 6>, 2> _piece_bb;
	Turn _turn;

	std::array<CastlingRights, 2> _castling_rights;

	Square _ep_square;

	uint8_t _halfmove_count;
	uint16_t _fullmove_count;

	std::string _cur_fen;
};

template <enumColor Side>
INLINE bool CastlingRights::notThroughCheck_Short(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::f1 : Square::f8;
	return !pos.attacked_KingIncluded(IntermediateSq);
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughCheck_Long(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::d1 : Square::d8;
	return !pos.attacked_KingIncluded(IntermediateSq);
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughPieces_Short(const Position& pos) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::f1) | BitBoard(Square::g1)
		: BitBoard(Square::f8) | BitBoard(Square::g8);

	return !(pos.getOccupied() & Intermediates);
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughPieces_Long(const Position& pos) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::b1) | BitBoard(Square::c1) | BitBoard(Square::d1)
		: BitBoard(Square::b8) | BitBoard(Square::c8) | BitBoard(Square::d8);

	return !(pos.getOccupied() & Intermediates);
}

INLINE BitBoard Position::getByColor(enumColor col_type) const {
	return _piece_bb[col_type][Piece::PAWN]
		| _piece_bb[col_type][Piece::KNIGHT]
		| _piece_bb[col_type][Piece::BISHOP]
		| _piece_bb[col_type][Piece::ROOK]
		| _piece_bb[col_type][Piece::QUEEN]
		| _piece_bb[col_type][Piece::KING];
}

INLINE void Position::clearPieces() {
	for (enumColor col : { WHITE, BLACK })
		_piece_bb[col].fill(BitBoard::empty);
}

template <Piece::enumType Piece, enumColor Color>
INLINE BitBoard Position::get() const {
	if constexpr (Piece == Piece::PAWN)
		return getPawnsBySide(Color);
	else if constexpr (Piece == Piece::KNIGHT)
		return getKnightsBySide(Color);
	else if constexpr (Piece == Piece::BISHOP)
		return getBishopsBySide(Color);
	else if constexpr (Piece == Piece::ROOK)
		return getRooksBySide(Color);
	else if constexpr (Piece == Piece::QUEEN)
		return getQueensBySide(Color);

	return getKingBySide(Color);
}

INLINE bool Position::attacked(Square sq) const  {
	if (pawnAttacks(sq, _turn) & getPawnsBySide(!_turn)) 
		return true;
	if (knightAttacks(sq) & getKnightsBySide(!_turn)) 
		return true;
	if (SlidersMagics::bishopAttacks(sq, getOccupied()) & getBishopsQueens(!_turn)) 
		return true;
	return (SlidersMagics::rookAttacks(sq, getOccupied()) & getRooksQueens(!_turn));
}

INLINE bool Position::attacked_KingIncluded(Square sq) const {
	return attacked(sq) or (kingAttacks(sq) & getKingBySide(!_turn));
}

INLINE bool Position::isCheck() const {
	const Square own_king_sq = getKingBySide(_turn).bitScanForward();
	return attacked(own_king_sq);
}

INLINE Piece::enumType Position::pieceTypeOn(Square sq, enumColor by_color) const {
	for (Piece::enumType piece_t : Piece::piece_list) {
		if (_piece_bb[by_color][piece_t].getBit(sq))
			return piece_t;
	}

	return Piece::NONE;
}

INLINE Piece::enumType Position::pieceTypeOn(Square sq) const {
	const Piece::enumType white = pieceTypeOn(sq, WHITE);
	return white != Piece::NONE ? white : pieceTypeOn(sq, BLACK);
}