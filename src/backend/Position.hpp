/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "BitBoard.hpp"
#include "Piece.hpp"
#include "Attacks.hpp"
#include "Hash.hpp"
#include "Color.hpp"
#include "Move.hpp"
#include "Memory.hpp"

class Position;
struct NodeInfo;

namespace nn { 
	struct AccumulatorCache; 
}

static constexpr std::string_view StartposFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static constexpr std::string_view KiwipeteFEN = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

// wrapper around castling rights for single player
class CastlingRights {
public:
	CastlingRights() = default;
	explicit CastlingRights(bool kinit, bool qinit);

	bool operator==(const CastlingRights& rights) const;
	bool operator!=(const CastlingRights& rights) const;

	void printByColor(enumColor col_type) const;

	_NODISCARD _INLINE bool isShortPossible() const {
		return _kingside;
	}

	_NODISCARD _INLINE bool isLongPossible() const {
		return _queenside;
	}

	_NODISCARD _INLINE bool isAnyPossible() const {
		return _kingside or _queenside;
	}

	template <enumColor Side>
	_NODISCARD bool notThroughCheck_Short(const Position& pos) const;
	_NODISCARD bool notThroughCheck_Short(const Position& pos, enumColor side) const;

	template <enumColor Side>
	_NODISCARD bool notThroughCheck_Long(const Position& pos) const;
	_NODISCARD bool notThroughCheck_Long(const Position& pos, enumColor side) const;

	template <enumColor Side>
	_NODISCARD bool notThroughPieces_Short(BitBoard occupied) const;
	_NODISCARD bool notThroughPieces_Short(BitBoard occupied, enumColor side) const;

	template <enumColor Side>
	_NODISCARD bool notThroughPieces_Long(BitBoard occupied) const;
	_NODISCARD bool notThroughPieces_Long(BitBoard occupied, enumColor side) const;

	_INLINE void setKingSide(bool flag) {
		_kingside = flag;
	}

	_INLINE void setQueenSide(bool flag) {
		_queenside = flag;
	}

	_INLINE void clear() {
		_kingside = false, _queenside = false;
	}
private:
	bool _kingside, 
		 _queenside;
};

namespace Utils { class ExtPackedPosition; }

/* Tunable Static Exchange Evaluation piece values.
*  King score is fixed and not tuned.
*/

inline _P_CONSTEXPR int SeePawnValue = 100;
inline _P_CONSTEXPR int SeeKnightValue = 300;
inline _P_CONSTEXPR int SeeBishopValue = 300;
inline _P_CONSTEXPR int SeeRookValue = 500;
inline _P_CONSTEXPR int SeeQueenValue = 900;
inline constexpr int 	SeeKingValue = 12000;

// internal board state, including piece distribution 
// and game flags like castling
class Position {
public:
	friend class Utils::ExtPackedPosition;
	struct IrreversibleState;

	Position();
	explicit Position(std::string init_fen);
	explicit Position(std::string_view init_fen);

	void setByFEN(std::string fen);
	void setStartingPos();

	_NODISCARD std::string createFEN() const;

    enum enumStatusFlag {
        POSITION_NO_ERROR          = 0,
        POSITION_HASH_INVALID      = 1,
        POSITION_OCC_INVALID       = 2,
        POSITION_CASTLING_INVALID  = 3,
        POSITION_KING_INVALID      = 4,
        POSITION_PIECE_CNT_INVALID = 5,
    };

    // simplified check if a position is valid
    _NODISCARD bool isValid() const;

	_NODISCARD enumStatusFlag getErrFlag() const;

	_NODISCARD bool isQuiet();

	void print(std::ostream& os = std::cout) const;

	bool operator==(const Position& pos) const;
	bool operator!=(const Position& pos) const;

	_NODISCARD _INLINE BitBoard getPawnsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::PAWN];
	}

	_NODISCARD _INLINE BitBoard getKnightsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::KNIGHT];
	}

	_NODISCARD _INLINE BitBoard getBishopsBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::BISHOP];
	}

	_NODISCARD _INLINE BitBoard getRooksBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::ROOK];
	}

	_NODISCARD _INLINE BitBoard getQueensBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::QUEEN];
	}

	_NODISCARD _INLINE BitBoard getBishopsQueensBySide(enumColor col_type) const {
		return getBishopsBySide(col_type) | getQueensBySide(col_type);
	}

	_NODISCARD _INLINE BitBoard getRooksQueensBySide(enumColor col_type) const {
		return getRooksBySide(col_type) | getQueensBySide(col_type);
	}

	_NODISCARD _INLINE BitBoard getKingBySide(enumColor col_type) const {
		return _piece_bb[col_type][Piece::KING];
	}

	_NODISCARD _INLINE Square getKingSquare(enumColor col_type) const {
		assert(_king_sq[col_type] == _piece_bb[col_type][Piece::KING].bitScanForward());
		return _king_sq[col_type];
	}

	_NODISCARD _INLINE BitBoard getBySide(enumColor col_type) const {
		assert(_occupied[col_type] == getBySideOnFly(col_type));
		return _occupied[col_type];
	}

	_NODISCARD BitBoard getBySideOnFly(enumColor col_type) const;

	_NODISCARD _INLINE BitBoard getWhites() const {
		return getBySide(WHITE);
	}

	_NODISCARD _INLINE BitBoard getBlacks() const {
		return getBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getPawns() const {
		return getPawnsBySide(WHITE) | getPawnsBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getKnights() const {
		return getKnightsBySide(WHITE) | getKnightsBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getBishops() const {
		return getBishopsBySide(WHITE) | getBishopsBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getRooks() const {
		return getRooksBySide(WHITE) | getRooksBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getQueens() const {
		return getQueensBySide(WHITE) | getQueensBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getKings() const {
		return getKingBySide(WHITE) | getKingBySide(BLACK);
	}

	_NODISCARD _INLINE BitBoard getOccupied() const {
		return getWhites() | getBlacks();
	}

	_NODISCARD _INLINE BitBoard getOppositePieces() const {
		return getBySide(!_turn);
	}

	_NODISCARD _INLINE BitBoard getOwnPieces() const {
		return getBySide(_turn);
	}

	_NODISCARD _INLINE BitBoard getEmpties() const {
		return ~getOccupied();
	}

	_NODISCARD _INLINE Turn getTurn() const {
		return _turn;
	}

	_NODISCARD _INLINE Turn getOppositeTurn() const {
		return !_turn;
	}

	_NODISCARD _INLINE Square getEnPassantSq() const {
		return _ep_square;
	}

	template <Piece::enumType Piece, enumColor Color>
	_NODISCARD BitBoard get() const;

	_NODISCARD BitBoard get(Piece::enumType piece, enumColor color) const;

	_NODISCARD _INLINE CastlingRights getCastlingByColor(enumColor col_type) const {
		return _castling_rights[col_type];
	}

	_NODISCARD _INLINE CastlingRights getOwnCastling() const {
		return _castling_rights[_turn];
	}

	_NODISCARD _INLINE uint8_t getHalfmoveClock() const {
		return _halfmove_count;
	}

	_NODISCARD _INLINE uint16_t getFullmoveClock() const {
		return _fullmove_count;
	}

	_INLINE void setTurn(enumColor col_to_move) {
		_turn = col_to_move;
	}

	_NODISCARD _INLINE int getPiecesCount() const {
		return getOccupied().popCount();
	}

	_NODISCARD int getOnBoardMaterial(enumColor side) const;
	_NODISCARD int getOnBoardMaterial() const;

	_NODISCARD int getNonPawnMaterial(enumColor side) const;
	_NODISCARD int getNonPawnMaterial() const;

	// returns true whether square is attacked by any opposide-color piece excluding enemy king
	_NODISCARD bool attacked(Square sq, enumColor side) const;

	// just like attacked function above, but includes king attacks
	_NODISCARD bool attacked_KingIncluded(Square sq, enumColor side) const;

	_NODISCARD BitBoard attacksTo(Square sq, enumColor side, BitBoard occ) const;

	_NODISCARD bool isInCheck(enumColor side) const;
	_NODISCARD bool isInDoubleCheck(enumColor side) const;

	_NODISCARD BitBoard leastValuableAttackers(Square sq, enumColor side) const;

	// Do not return all of the checkers, but terminates as soon as just one checker in found.
	// If no checkers found, returns empty board.
	_NODISCARD BitBoard getCheckers(enumColor side) const;

	_NODISCARD Piece::enumType pieceOn(Square sq, enumColor by_color) const;
	_NODISCARD Piece fullPieceOn(Square sq) const;

	// returns whether move is legal or pseudo-legal
	bool make(Move32b& move);
	bool make(Move32b& move, nn::AccumulatorCache* accum_cache);
	void unmake(Move32b move, const IrreversibleState& prev_state);

	void makeNull(IrreversibleState& state, nn::AccumulatorCache* accum_cache);
	void unmakeNull(const IrreversibleState& prev_state);

	_NODISCARD uint64_t likelyZobristKeyAfterMove(Move32b& move) const;

	_NODISCARD _INLINE uint64_t getZobristKey() const {
		return _zhash;
	}

	template <bool Root = true>
	uint64_t perft(unsigned depth);

	template <bool ExactScore>
	int staticExchangeEval(Square org, 
						   Square sq, 
						   Piece::enumType target, 
						   Piece::enumType att) const;

	_NODISCARD IrreversibleState getIrreversibleState() const;

	struct IrreversibleState {
		Square 					   ep_sq;
		uint8_t 				   halfmove_count;
		array1d<CastlingRights, 2> castling_rights;
		// It is not really required to store previous hash key,
		// since it can be recomputed. But keep it here for simplicity and efficiency.
		uint64_t 				   hash_key;
	};
private:
	void clearPieces();
	void setGameStatesFromStr(const std::string fen, size_t i);

	BitBoard getWeakestAttacker(BitBoard bb,
								enumColor side,
								Piece::uint_t& piece) const;

	array1d<array1d<BitBoard, 6>, 2> _piece_bb;
	array1d<BitBoard, 2> 			 _occupied;
	array1d<CastlingRights, 2> 		 _castling_rights;
	array1d<Square, 2> 				 _king_sq;
	Turn 							 _turn;
	Square 	    					 _ep_square;
	ZHash 						   	 _zhash;
	uint8_t     					 _halfmove_count;
	uint16_t    					 _fullmove_count;
};

_INLINE bool CastlingRights::operator==(const CastlingRights& rights) const {
	return _queenside == rights._queenside
		and _kingside == rights._kingside;
}

_INLINE bool CastlingRights::operator!=(const CastlingRights& rights) const {
	return !(*this == rights);
}

template <enumColor Side>
_INLINE bool CastlingRights::notThroughCheck_Short(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::SQ_F1 : Square::SQ_F8;
	static constexpr Square KingDstSq = Side == WHITE ? Square::SQ_G1 : Square::SQ_G8;

	return !pos.attacked_KingIncluded(IntermediateSq, Side)
		and !(kingAttacks(KingDstSq) & pos.getKingBySide(!Side));
}

_INLINE bool CastlingRights::notThroughCheck_Short(const Position& pos, enumColor side) const {
	const Square intermediate_sq = side == WHITE ? Square::SQ_F1 : Square::SQ_F8;
	const Square king_dst_sq = side == WHITE ? Square::SQ_G1 : Square::SQ_G8;

	return !pos.attacked_KingIncluded(intermediate_sq, side)
		and !(kingAttacks(king_dst_sq) & pos.getKingBySide(!side));
}

template <enumColor Side>
_INLINE bool CastlingRights::notThroughCheck_Long(const Position& pos) const {
	static constexpr Square IntermediateSq = Side == WHITE ? Square::SQ_D1 : Square::SQ_D8;
	static constexpr Square KingDstSq = Side == WHITE ? Square::SQ_C1 : Square::SQ_C8;

	return !pos.attacked_KingIncluded(IntermediateSq, Side)
		and !(kingAttacks(KingDstSq) & pos.getKingBySide(!Side));
}

_INLINE bool CastlingRights::notThroughCheck_Long(const Position& pos, enumColor side) const {
	const Square intermediate_sq = side == WHITE ? Square::SQ_D1 : Square::SQ_D8;
	const Square king_dst_sq = side == WHITE ? Square::SQ_C1 : Square::SQ_C8;

	return !pos.attacked_KingIncluded(intermediate_sq, side)
		and !(kingAttacks(king_dst_sq) & pos.getKingBySide(!side));
}

template <enumColor Side>
_INLINE bool CastlingRights::notThroughPieces_Short(BitBoard occupied) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::SQ_F1) | BitBoard(Square::SQ_G1)
		: BitBoard(Square::SQ_F8) | BitBoard(Square::SQ_G8);
	
	return !(occupied & Intermediates);
}

_INLINE bool CastlingRights::notThroughPieces_Short(BitBoard occupied, enumColor side) const {
	const BitBoard intermediates = side == WHITE ?
		BitBoard(Square::SQ_F1) | BitBoard(Square::SQ_G1)
		: BitBoard(Square::SQ_F8) | BitBoard(Square::SQ_G8);

	return !(occupied & intermediates);
}

template <enumColor Side>
_INLINE bool CastlingRights::notThroughPieces_Long(BitBoard occupied) const {
	static constexpr BitBoard Intermediates = Side == WHITE ?
		BitBoard(Square::SQ_B1) | BitBoard(Square::SQ_C1) | BitBoard(Square::SQ_D1)
		: BitBoard(Square::SQ_B8) | BitBoard(Square::SQ_C8) | BitBoard(Square::SQ_D8);

	return !(occupied & Intermediates);
}

_INLINE bool CastlingRights::notThroughPieces_Long(BitBoard occupied, enumColor side) const {
	const BitBoard intermediates = side == WHITE ?
		BitBoard(Square::SQ_B1) | BitBoard(Square::SQ_C1) | BitBoard(Square::SQ_D1)
		: BitBoard(Square::SQ_B8) | BitBoard(Square::SQ_C8) | BitBoard(Square::SQ_D8);

	return !(occupied & intermediates);
}

_INLINE bool Position::operator!=(const Position& pos) const {
	return !(*this == pos);
}

_INLINE BitBoard Position::getBySideOnFly(enumColor col_type) const {
	return _piece_bb[col_type][Piece::PAWN]
		| _piece_bb[col_type][Piece::KNIGHT]
		| _piece_bb[col_type][Piece::BISHOP]
		| _piece_bb[col_type][Piece::ROOK]
		| _piece_bb[col_type][Piece::QUEEN]
		| _piece_bb[col_type][Piece::KING];
}

_INLINE Position::IrreversibleState Position::getIrreversibleState() const {
	return Position::IrreversibleState{ _ep_square, 
										_halfmove_count,
										_castling_rights, 
										getZobristKey() };
}

_INLINE void Position::clearPieces() {
	mem::memSet(_piece_bb[0].data(), 0, 2 * 6 * sizeof(BitBoard));
	mem::memSet(_occupied.data(), 0, 2 * sizeof(BitBoard));
}

template <Piece::enumType Piece, enumColor Color>
_INLINE BitBoard Position::get() const {
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

_INLINE BitBoard Position::get(Piece::enumType piece, enumColor color) const {
	if (piece == Piece::PAWN)
		return getPawnsBySide(color);
	else if (piece == Piece::KNIGHT)
		return getKnightsBySide(color);
	else if (piece == Piece::BISHOP)
		return getBishopsBySide(color);
	else if (piece == Piece::ROOK)
		return getRooksBySide(color);
	else if (piece == Piece::QUEEN)
		return getQueensBySide(color);

	return getKingBySide(color);
}

_INLINE bool Position::attacked(Square sq, enumColor side) const {
	const BitBoard occ = getOccupied();
	return (knightAttacks(sq) & getKnightsBySide(!side)) or
		(pawnAttacks(sq, side) & getPawnsBySide(!side)) or
		(SlidersAttacks::rookAttacks(sq, occ) & getRooksQueensBySide(!side)) or
		(SlidersAttacks::bishopAttacks(sq, occ) & getBishopsQueensBySide(!side));
}

_INLINE bool Position::attacked_KingIncluded(Square sq, enumColor side) const {
	return attacked(sq, side) or (kingAttacks(sq) & getKingBySide(!side));
}

_INLINE BitBoard Position::attacksTo(Square sq, enumColor side, BitBoard occ) const {
	const BitBoard queen = _piece_bb[!side][Piece::QUEEN],
				   rookQueen = _piece_bb[!side][Piece::ROOK] | queen,
				   bishopQueen = _piece_bb[!side][Piece::BISHOP] | queen;

	return (_piece_bb[!side][Piece::PAWN] & pawnAttacks(sq, side))
		| (_piece_bb[!side][Piece::KNIGHT] & knightAttacks(sq))
		| (_piece_bb[!side][Piece::KING] & kingAttacks(sq))
		| (bishopQueen & attacks<Piece::BISHOP>(sq, occ))
		| (rookQueen & attacks<Piece::ROOK>(sq, occ));
}

_INLINE bool Position::isInCheck(enumColor side) const {
	return attacked(getKingSquare(side), side);
}

_INLINE bool Position::isInDoubleCheck(enumColor side) const {
	const Square king_sq = getKingSquare(side);
	const BitBoard occupied = getOccupied();

	uint8_t att_count = 0;

	if (SlidersAttacks::bishopAttacks(king_sq, occupied) & getBishopsQueensBySide(!side))
		att_count++;

	if (att_count >= 2) return true;

	if (SlidersAttacks::rookAttacks(king_sq, occupied) & getRooksQueensBySide(!side))
		att_count++;

	if (att_count >= 2) return true;

	if (pawnAttacks(king_sq, side) & getPawnsBySide(!side))
		att_count++;

	if (knightAttacks(king_sq) & getKnightsBySide(!side))
		att_count++;

	return att_count >= 2;
}

_INLINE BitBoard Position::leastValuableAttackers(Square sq, enumColor attacked) const {
	const BitBoard occupied = getOccupied();
	BitBoard bb;
	
	bb = pawnAttacks(sq, attacked) & getPawnsBySide(!attacked);
	if (bb)
		return bb;

	bb = knightAttacks(sq) & getKnightsBySide(!attacked);
	if (bb)
		return bb;

	bb = SlidersAttacks::bishopAttacks(sq, occupied) & getBishopsQueensBySide(!attacked);
	if (bb)
		return bb;

	bb = SlidersAttacks::rookAttacks(sq, occupied) & getRooksQueensBySide(!attacked);
	if (bb)
		return bb;

	return BitBoard(0_ui64);
}

_INLINE BitBoard Position::getCheckers(enumColor side) const {
	return leastValuableAttackers(getKingSquare(side), side);
}

_INLINE Piece::enumType Position::pieceOn(Square sq, enumColor by_color) const {
	for (Piece::enumType piece_t : Piece::PieceTypeList) {
		if (_piece_bb[by_color][piece_t].isOccupiedSq(sq))
			return piece_t;
	}

	return Piece::NONE;
}

_INLINE Piece Position::fullPieceOn(Square sq) const {
	const Piece::enumType type = pieceOn(sq, WHITE);
	return type != Piece::NONE ? Piece(WHITE, type) : Piece(BLACK, pieceOn(sq, BLACK));
}

_INLINE bool operator!(Position::enumStatusFlag err_flag) {
    return err_flag != Position::POSITION_NO_ERROR;
}

_INLINE std::string toStr(Position::enumStatusFlag err_flag) {
    switch (err_flag) {
    case Position::POSITION_NO_ERROR:
        return "no error";
    case Position::POSITION_HASH_INVALID:
        return "invalid cache";
    case Position::POSITION_OCC_INVALID:
        return "invalid occupancy";
    case Position::POSITION_CASTLING_INVALID:
        return "invalid castling";
    case Position::POSITION_KING_INVALID:
        return "invalid king";
    case Position::POSITION_PIECE_CNT_INVALID:
        return "invalid piece number";
    }

    return "";
}

template <bool ExactScore>
int _StaticExchangeEval_unittest(const Position& pos, 
								 Square org, 
								 Square sq, 
								 Piece::enumType target, 
								 Piece::enumType attacker);
