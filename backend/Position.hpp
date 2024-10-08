#pragma once

#include "BitBoard.hpp"
#include "Piece.hpp"
#include "Attacks.hpp"
#include "Hash.hpp"
#include "Color.hpp"

class Move;
class Position;

// wrapper around castling rights for single player
class CastlingRights {
public:
	CastlingRights() = default;
	CastlingRights(bool kinit, bool qinit);

	void printByColor(enumColor col_type) const;

	INLINE bool isShortPossible() const {
		return _kingside;
	}

	INLINE bool isLongPossible() const {
		return _queenside;
	}

	template <enumColor Side>
	bool notThroughCheck_Short(const Position& pos) const;
	bool notThroughCheck_Short(const Position& pos, enumColor side) const;

	template <enumColor Side>
	bool notThroughCheck_Long(const Position& pos) const;
	bool notThroughCheck_Long(const Position& pos, enumColor side) const;

	template <enumColor Side>
	bool notThroughPieces_Short(BitBoard occupied) const;
	bool notThroughPieces_Short(BitBoard occupied, enumColor side) const;

	template <enumColor Side>
	bool notThroughPieces_Long(BitBoard occupied) const;
	bool notThroughPieces_Long(BitBoard occupied, enumColor side) const;

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
	struct IrreversibleState;

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

	INLINE Square getKingSquare(enumColor col_type) const {
		return _king_sq[col_type];
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

	INLINE BitBoard getOwnPieces() const {
		return getByColor(_turn);
	}

	INLINE BitBoard getEmpties() const {
		return ~getOccupied();
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

	INLINE CastlingRights getOwnCastling() const {
		return _castling_rights[_turn];
	}

	INLINE uint8_t halfmoveClock() const {
		return _halfmove_count;
	}

	INLINE void setTurn(enumColor col_to_move) {
		_turn = col_to_move;
	}

	// returns true whether square is attacked by any opposide-color piece excluding enemy king
	bool attacked(Square sq, enumColor side) const;

	// just like attacked function above, but includes king attacks
	bool attacked_KingIncluded(Square sq, enumColor side) const;

	BitBoard attacksTo(Square sq, enumColor side, BitBoard occ) const;

	bool isInCheck(enumColor side) const;
	bool isInDoubleCheck(enumColor side) const;

	BitBoard leastValuableAttackers(Square sq, enumColor side) const;

	// Do not return all of the checkers, but terminates as soon as just one checker in found.
	// If no checkers found, returns empty board.
	BitBoard getCheckers(enumColor side) const;

	Piece::enumType pieceTypeOn(Square sq, enumColor by_color) const;
	Piece pieceOn(Square sq) const;

	// returns whether move is legal or pseudo-legal
	bool make(Move& move, IrreversibleState& state);
	void unmake(Move move, const IrreversibleState& prev_state);

	void makeNull(IrreversibleState& state);
	void unmakeNull(const IrreversibleState& prev_state);

	uint64_t getZobristKey() const;

	template <bool Root = true>
	uint64_t perft(unsigned depth);

	int StaticExchangeEval(const Square sq) const;

	struct IrreversibleState {
		Square ep_sq;
		uint8_t halfmove_count;
		std::array<CastlingRights, 2> castling_rights;
		// TEMPORARY
		uint64_t hash_key;
	};

	static constexpr std::string_view starting_fen 
		= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
private:
	void clearPieces();
	void setGameStatesFromStr(const std::string fen, int i);

	//BitBoard leastValuableAttacker_withMask(const Square sq, enumColor side, 
	//BitBoard occupied, BitBoard mask, Piece::enumType& attacker) const;

	std::array<std::array<BitBoard, 6>, 2> _piece_bb;
	Turn _turn;

	std::array<CastlingRights, 2> _castling_rights;
	Square _ep_square;
	uint8_t _halfmove_count;

	uint16_t _fullmove_count;

	std::array<Square, 2> _king_sq;

	ZobristHash _hashing;
};

template <enumColor Side>
INLINE bool CastlingRights::notThroughCheck_Short(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::f1 : Square::f8;
	return !pos.attacked_KingIncluded(IntermediateSq, pos.getTurn());
}

INLINE bool CastlingRights::notThroughCheck_Short(const Position& pos, enumColor side) const {
	const Square IntermediateSq = side == WHITE ? Square::f1 : Square::f8;
	return !pos.attacked_KingIncluded(IntermediateSq, pos.getTurn());
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughCheck_Long(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::d1 : Square::d8;
	return !pos.attacked_KingIncluded(IntermediateSq, pos.getTurn());
}

INLINE bool CastlingRights::notThroughCheck_Long(const Position& pos, enumColor side) const {
	const Square IntermediateSq = side == WHITE ? Square::d1 : Square::d8;
	return !pos.attacked_KingIncluded(IntermediateSq, pos.getTurn());
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughPieces_Short(BitBoard occupied) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::f1) | BitBoard(Square::g1)
		: BitBoard(Square::f8) | BitBoard(Square::g8);
	
	return !(occupied & Intermediates);
}

INLINE bool CastlingRights::notThroughPieces_Short(BitBoard occupied, enumColor side) const {
	const BitBoard Intermediates = side == WHITE ?
		BitBoard(Square::f1) | BitBoard(Square::g1)
		: BitBoard(Square::f8) | BitBoard(Square::g8);

	return !(occupied & Intermediates);
}

template <enumColor Side>
INLINE bool CastlingRights::notThroughPieces_Long(BitBoard occupied) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::b1) | BitBoard(Square::c1) | BitBoard(Square::d1)
		: BitBoard(Square::b8) | BitBoard(Square::c8) | BitBoard(Square::d8);

	return !(occupied & Intermediates);
}

INLINE bool CastlingRights::notThroughPieces_Long(BitBoard occupied, enumColor side) const {
	const BitBoard Intermediates = side == WHITE ?
		BitBoard(Square::b1) | BitBoard(Square::c1) | BitBoard(Square::d1)
		: BitBoard(Square::b8) | BitBoard(Square::c8) | BitBoard(Square::d8);

	return !(occupied & Intermediates);
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

INLINE bool Position::attacked(Square sq, enumColor side) const  {
	if (pawnAttacks(sq, side) & getPawnsBySide(!side)) 
		return true;
	if (knightAttacks(sq) & getKnightsBySide(!side)) 
		return true;
	if (SlidersMagics::bishopAttacks(sq, getOccupied()) & getBishopsQueens(!side))
		return true;
	return (SlidersMagics::rookAttacks(sq, getOccupied()) & getRooksQueens(!side));
}

INLINE bool Position::attacked_KingIncluded(Square sq, enumColor side) const {
	return attacked(sq, side) or (kingAttacks(sq) & getKingBySide(!side));
}

INLINE BitBoard Position::attacksTo(Square sq, enumColor side, BitBoard occ) const {
	const BitBoard queen = _piece_bb[!side][Piece::QUEEN],
		rookQueen = _piece_bb[!side][Piece::ROOK] | queen,
		bishopQueen = _piece_bb[!side][Piece::BISHOP] | queen;

	return (_piece_bb[!side][Piece::PAWN] & pawnAttacks(sq, side))
		| (_piece_bb[!side][Piece::KNIGHT] & knightAttacks(sq))
		| (_piece_bb[!side][Piece::KING] & kingAttacks(sq))
		| (bishopQueen & attacks<Piece::BISHOP>(sq, occ))
		| (rookQueen & attacks<Piece::ROOK>(sq, occ));
}

INLINE bool Position::isInCheck(enumColor side) const {
	const Square king_sq = getKingSquare(side);
	return attacked(king_sq, side);
}

INLINE bool Position::isInDoubleCheck(enumColor side) const {
	const Square king_sq = getKingSquare(side);
	const BitBoard occupied = getOccupied();

	uint8_t att_count = 0;

	if (pawnAttacks(king_sq, side) & getPawnsBySide(!side))
		att_count++;

	if (knightAttacks(king_sq) & getKnightsBySide(!side))
		att_count++;

	if (att_count >= 2) return true;

	if (SlidersMagics::bishopAttacks(king_sq, occupied) & getBishopsQueens(!side))
		att_count++;

	if (att_count >= 2) return true;

	if (SlidersMagics::rookAttacks(king_sq, occupied) & getRooksQueens(!side))
		att_count++;

	return att_count >= 2;
}

INLINE BitBoard Position::leastValuableAttackers(Square sq, enumColor attacked) const {
	const BitBoard occupied = getOccupied();
	BitBoard bb;
	
	bb = pawnAttacks(sq, attacked) & getPawnsBySide(!attacked);
	if (bb)
		return bb;

	bb = knightAttacks(sq) & getKnightsBySide(!attacked);
	if (bb)
		return bb;

	bb = SlidersMagics::bishopAttacks(sq, occupied) & getBishopsQueens(!attacked);
	if (bb)
		return bb;

	bb = SlidersMagics::rookAttacks(sq, occupied) & getRooksQueens(!attacked);
	if (bb)
		return bb;

	return BitBoard::empty;
}

INLINE BitBoard Position::getCheckers(enumColor side) const {
	return leastValuableAttackers(getKingSquare(side), side);
}

INLINE Piece::enumType Position::pieceTypeOn(Square sq, enumColor by_color) const {
	for (Piece::enumType piece_t : Piece::piece_list) {
		if (_piece_bb[by_color][piece_t].isOccupiedSq(sq))
			return piece_t;
	}

	return Piece::NONE;
}

INLINE Piece Position::pieceOn(Square sq) const {
	const Piece::enumType type = pieceTypeOn(sq, WHITE);
	return type != Piece::NONE ? Piece(WHITE, type) : Piece(BLACK, pieceTypeOn(sq, BLACK));
}

INLINE uint64_t Position::getZobristKey() const {
	return _hashing._key;
}