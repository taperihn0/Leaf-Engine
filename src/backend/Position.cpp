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

#include "Position.hpp"
#include "Move.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "Search.hpp"
#include "Accumulator.hpp"
#include "StaticEval.hpp"

#include <sstream>

CastlingRights::CastlingRights(bool kinit, bool qinit) 
	: _kingside(kinit)
	, _queenside(qinit) 
{}

void CastlingRights::printByColor(enumColor col_type) const {
	std::string msg;
	if (_kingside) msg += col_type == BLACK ? 'k' : 'K';
	if (_queenside) msg += col_type == BLACK ? 'q' : 'Q';
	std::cout << msg;
}

Position::Position() {
	std::fill(_piece_bb[0].begin(), _piece_bb[0].end(), 0);
	std::fill(_piece_bb[1].begin(), _piece_bb[1].end(), 0);
	_occupied[0] = 0_ui64;
	_occupied[1] = 0_ui64;
	_zhash = 0_ui64;
}

Position::Position(std::string init_fen) {
	setByFEN(init_fen); 
}

Position::Position(std::string_view init_fen) 
	: Position(static_cast<std::string>(init_fen))
{}

void Position::setByFEN(std::string fen) {
	size_t first = fen.find_first_of("pnbrqkPNBRQK12345678");

	clearPieces();
	
	int x = 0, y = 7;

	for (size_t i = first; i < size(fen); i++) {
		const char c = fen[i];

		if (isdigit(c)) {
			x += c - '0';
			continue;
		}
		else if (c == ' ') {
			setGameStatesFromStr(fen, i + 1);
			break;
		}

		int in = y * 8 + x;
		if (c == '/') {
			y--, x = 0;
			continue;
		}

		const enumColor col = islower(c) ? BLACK : WHITE;
		_piece_bb[col][Piece::fromChar(col, c).value()].setBit(in);
		++x;
	}

	_occupied[WHITE] = getBySideOnFly(WHITE);
	_occupied[BLACK] = getBySideOnFly(BLACK);

	_king_sq[WHITE] = getKingBySide(WHITE).bitScanForward();
	_king_sq[BLACK] = getKingBySide(BLACK).bitScanForward();
}

void Position::setStartingPos() {
	setByFEN(static_cast<std::string>(StartposFEN));
}

std::string Position::createFEN() const {
	std::stringstream fen;

    for (int y = 7; y >= 0; --y) {
        int empty_count = 0;
        for (int x = 0; x < 8; ++x) {
            int sq = y * 8 + x;
            bool found = false;

            for (enumColor col : { WHITE, BLACK }) {
                for (int p_type = 0; p_type < 6; ++p_type) {
                    if (_piece_bb[col][p_type].isOccupiedSq(sq)) {
                        if (empty_count > 0) {
                            fen << empty_count;
                            empty_count = 0;
                        }

						fen << Piece(col, static_cast<Piece::enumType>(p_type));
						
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }

            if (!found) {
                empty_count++;
            }
        }

        if (empty_count > 0) {
            fen << empty_count;
        }

        if (y > 0) {
            fen << '/';
        }
    }

	fen << ' ' << "wb"[_turn] << ' ';

	if (_castling_rights[WHITE].isAnyPossible() or
	    _castling_rights[BLACK].isAnyPossible()) {
		for (enumColor side : { WHITE, BLACK }) {
			if (!_castling_rights[side].isAnyPossible())
				continue;

			if (_castling_rights[side].isShortPossible())
				fen << "Kk"[side];
			if (_castling_rights[side].isLongPossible())
				fen << "Qq"[side];
		}
	} 
	else fen << '-';

	fen << ' ';
	_ep_square.print(fen);

	fen << ' ' << static_cast<int>(_halfmove_count) 
		<< ' ' << static_cast<int>(_fullmove_count);

	assert(Position(fen.str()) == *this);
	return fen.str();
}

bool Position::isValid() const {
    return getErrFlag() == POSITION_NO_ERROR;
}

Position::enumStatusFlag Position::getErrFlag() const {
    if (_zhash != ZHash::generateOnFly(*this))
        return POSITION_HASH_INVALID;

    else if (getBySide(WHITE) != getBySideOnFly(WHITE)
          or getBySide(BLACK) != getBySideOnFly(BLACK))
        return POSITION_OCC_INVALID;

    else if ((getCastlingByColor(WHITE).isLongPossible()  and !getRooksBySide(WHITE).isOccupiedSq(Square::SQ_A1))
          or (getCastlingByColor(WHITE).isShortPossible() and !getRooksBySide(WHITE).isOccupiedSq(Square::SQ_H1))
          or (getCastlingByColor(BLACK).isLongPossible()  and !getRooksBySide(BLACK).isOccupiedSq(Square::SQ_A8))
          or (getCastlingByColor(BLACK).isShortPossible() and !getRooksBySide(BLACK).isOccupiedSq(Square::SQ_H8)))
        return POSITION_CASTLING_INVALID;

    else if (_king_sq[WHITE] != getKingBySide(WHITE).bitScanForward()
          or _king_sq[BLACK] != getKingBySide(BLACK).bitScanForward())
        return POSITION_KING_INVALID;

    else if (getOccupied().popCount() > 32)
        return POSITION_PIECE_CNT_INVALID;

    return POSITION_NO_ERROR;
}

bool Position::isQuiet() {
	return !MoveGen::isAnyCapture(*this);
}

void Position::print(std::ostream& os) const {
	os << "     A   B   C   D   E   F   G   H";

	for (int h = 7; h >= 0; h--) {
		os << "\n   +---+---+---+---+---+---+---+---+\n"
			<< ' ' << h + 1 << " | ";

		for (int i = 8 * h; i < 8 * (h + 1); i++) {
			fullPieceOn(i).print(os);
			os << " | ";
		}

		os << h + 1;
	}

	os << "\n   +---+---+---+---+---+---+---+---+\n"
	   << "     A   B   C   D   E   F   G   H\n\n"
	   << "FEN: ";

	const std::string fen = createFEN();

	os << fen << '\n';
}

bool Position::operator==(const Position& pos) const {
		 if (getOccupied() != pos.getOccupied())
		return false;

	else if (getTurn() != pos.getTurn())
		return false;

	else if (getCastlingByColor(WHITE) != pos.getCastlingByColor(WHITE)
	      or getCastlingByColor(BLACK) != pos.getCastlingByColor(BLACK))
		return false;
		
	else if (getEnPassantSq() != pos.getEnPassantSq())
		return false;

	else if (getHalfmoveClock() != pos.getHalfmoveClock()
	      or getFullmoveClock() != pos.getFullmoveClock())
		return false;
	
	else if (getKingSquare(WHITE) != pos.getKingSquare(WHITE)
		  or getKingBySide(BLACK) != pos.getKingBySide(BLACK))
		return false;

	else if (getZobristKey() != pos.getZobristKey())
		return false;

	for (enumColor col : { WHITE, BLACK}) {
		for (Piece::enumType piece : Piece::PieceTypeList) {
			if (get(piece, col) != pos.get(piece, col))
				return false;
		}
	}
	
	return true;
}

int Position::getOnBoardMaterial(enumColor side) const {
	return getPawnsBySide(side).popCount() * PawnValue + getNonPawnMaterial(side);
}

int Position::getOnBoardMaterial() const {
	return getOnBoardMaterial(WHITE) + getOnBoardMaterial(BLACK);
}

int Position::getNonPawnMaterial(enumColor side) const {
	return getQueensBySide(side).popCount() * QueenValue +
		   getRooksBySide(side).popCount() * RookValue +
		   getBishopsBySide(side).popCount() * BishopValue +
		   getKnightsBySide(side).popCount() * KnightValue;
}

int Position::getNonPawnMaterial() const {
	return getNonPawnMaterial(WHITE) + getNonPawnMaterial(BLACK);
}

bool Position::make(Move32b& move) {
	nn::AccumulatorCache tmp_accum_cache;
	return make(move, &tmp_accum_cache);
}

bool Position::make(Move32b& move, nn::AccumulatorCache* accum_cache) {
	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  promotion = move.isPromotion();
	const Piece::enumType piece_t = move.getPiece();
	const int			  dir = _turn == WHITE ? 8 : -8;
	const bool			  pawn_push = piece_t == Piece::PAWN and !capture,
						  double_pawn_push = pawn_push and (org - dst > 8 or dst - org > 8);

	// we've got double buffer for white- and black-perspective,
	// and for each perspectives maximum features (both removed and added) is 2.

	accum_cache->clearBuffers();

	size_t& added_feature_cnt = accum_cache->added_features_cnt;
	size_t& removed_feature_cnt = accum_cache->removed_features_cnt;

	assert(added_feature_cnt == 0 and removed_feature_cnt == 0);

	if (capture) {
		if (move.isEnPassant()) {
			assert(piece_t == Piece::PAWN);

			const Square cap_sq = dst - dir;

			_piece_bb[!_turn][Piece::PAWN].popBit(cap_sq);
			_occupied[!_turn].popBit(cap_sq);
			_zhash ^= ZHashMasks->piece_keys[!_turn][Piece::PAWN][cap_sq];

			accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(cap_sq, Piece::PAWN, !_turn);
		}
		else {
			const Piece::enumType captured = pieceOn(dst, !_turn);
			move.setCaptured(captured);

			assert(captured != Piece::NONE and captured != Piece::KING);

			_piece_bb[!_turn][captured].popBit(dst);
			_occupied[!_turn].popBit(dst);
			_zhash ^= ZHashMasks->piece_keys[!_turn][captured][dst];

			accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(dst, captured, !_turn);

			const Square right_corner_opp = _turn == BLACK ? Square::SQ_H1 : Square::SQ_H8,
						 left_corner_opp = _turn == BLACK ? Square::SQ_A1 : Square::SQ_A8;

			if (_castling_rights[!_turn].isShortPossible() and dst == right_corner_opp) {
				_zhash ^= ZHashMasks->short_castle_keys[!_turn];
				_castling_rights[!_turn].setKingSide(false);
			}
			else if (_castling_rights[!_turn].isLongPossible() and dst == left_corner_opp) {
				_zhash ^= ZHashMasks->long_castle_keys[!_turn];
				_castling_rights[!_turn].setQueenSide(false);
			}
		}
	}

	if (promotion) {
		const Piece::enumType promo_piece_t = move.getPromoPiece();
		assert(piece_t == Piece::PAWN and promo_piece_t != Piece::PAWN and promo_piece_t != Piece::KING);

		_piece_bb[_turn][piece_t].popBit(org);
		_piece_bb[_turn][promo_piece_t].setBit(dst);
		_occupied[_turn].moveBit(org, dst);

		_zhash ^= ZHashMasks->piece_keys[_turn][piece_t][org];
		_zhash ^= ZHashMasks->piece_keys[_turn][promo_piece_t][dst];

		accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(org, piece_t, _turn);
		accum_cache->added_features[added_feature_cnt++] = nn::FeatureData(dst, promo_piece_t, _turn);
	}
	else { // if not a promotion - just move a piece on its own bitboard 
		_piece_bb[_turn][piece_t].moveBit(org, dst);
		_occupied[_turn].moveBit(org, dst);

		_zhash ^= ZHashMasks->piece_keys[_turn][piece_t][org];
		_zhash ^= ZHashMasks->piece_keys[_turn][piece_t][dst];

		accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(org, piece_t, _turn);
		accum_cache->added_features[added_feature_cnt++] = nn::FeatureData(dst, piece_t, _turn);
	}

	if (piece_t == Piece::KING) {
		if (move.isShortCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst + 1, dst - 1);
			_occupied[_turn].moveBit(dst + 1, dst - 1);

			_zhash ^= ZHashMasks->piece_keys[_turn][Piece::ROOK][dst + 1];
			_zhash ^= ZHashMasks->piece_keys[_turn][Piece::ROOK][dst - 1];

			accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(dst + 1, Piece::ROOK, _turn);
			accum_cache->added_features[added_feature_cnt++] = nn::FeatureData(dst - 1, Piece::ROOK, _turn);
		}
		else if (move.isLongCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst - 2, dst + 1);
			_occupied[_turn].moveBit(dst - 2, dst + 1);

			_zhash ^= ZHashMasks->piece_keys[_turn][Piece::ROOK][dst - 2];
			_zhash ^= ZHashMasks->piece_keys[_turn][Piece::ROOK][dst + 1];

			accum_cache->removed_features[removed_feature_cnt++] = nn::FeatureData(dst - 2, Piece::ROOK, _turn);
			accum_cache->added_features[added_feature_cnt++] = nn::FeatureData(dst + 1, Piece::ROOK, _turn);
		}

		_king_sq[_turn] = dst;
	}

	accum_cache->markDirty();

	const bool legal = !isInCheck(_turn);
	move.setLegalMoved(legal);

	// Just leave castling flags untouched since the move is pseudo-legal.
	// It will be ignored anyway in the search.
	if (legal) {
		const Square right_corner = _turn == WHITE ? Square::SQ_H1 : Square::SQ_H8,
					 left_corner = _turn == WHITE ? Square::SQ_A1 : Square::SQ_A8;

		if (_castling_rights[_turn].isShortPossible() and 
			(piece_t == Piece::KING or getRooksBySide(_turn).isEmptySq(right_corner))) {
			_zhash ^= ZHashMasks->short_castle_keys[_turn];
			_castling_rights[_turn].setKingSide(false);
		}

		if (_castling_rights[_turn].isLongPossible() and 
			(piece_t == Piece::KING or getRooksBySide(_turn).isEmptySq(left_corner))) {
			_zhash ^= ZHashMasks->long_castle_keys[_turn];
			_castling_rights[_turn].setQueenSide(false);
		}

		// reset old en passant square state
		if (!_ep_square.isNull())
			_zhash ^= ZHashMasks->ep_file_keys[_ep_square.getFile()];

		_ep_square = Square::None;

		if (double_pawn_push) {
			_ep_square = dst - dir;
			_zhash ^= ZHashMasks->ep_file_keys[_ep_square.getFile()];
		}

		_zhash ^= ZHashMasks->black_key;

		_halfmove_count = capture or pawn_push or double_pawn_push ? 0 : _halfmove_count + 1;
	}
	
	_fullmove_count += static_cast<int>(_turn);
	_turn = !_turn;

	return legal;
}

void Position::unmake(Move32b move, const IrreversibleState& prev_state) {
	const Piece::enumType piece_t = move.getPiece();
	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  ep_capture = move.isEnPassant(),
						  promotion = move.isPromotion();

	_turn = !_turn;

	if (promotion) {
		const Piece::enumType promo_piece_t = move.getPromoPiece();

		assert(piece_t == Piece::PAWN and promo_piece_t != Piece::PAWN and promo_piece_t != Piece::KING);
		_piece_bb[_turn][piece_t].setBit(org);
		_piece_bb[_turn][promo_piece_t].popBit(dst);
		_occupied[_turn].moveBit(dst, org);
	}
	else { // if not a promotion - just move a piece to origin square
		_piece_bb[_turn][piece_t].moveBit(dst, org);
		_occupied[_turn].moveBit(dst, org);
	}

	if (capture) {
		if (ep_capture) {
			const int dir = _turn == WHITE ? 8 : -8;

			assert(piece_t == Piece::PAWN);
			_piece_bb[!_turn][Piece::PAWN].setBit(dst - dir);
			_occupied[!_turn].setBit(dst - dir);
		}
		else {
			const Piece::enumType captured = move.getCapturedMoved();

			assert(captured != Piece::NONE);
			_piece_bb[!_turn][captured].setBit(dst);
			_occupied[!_turn].setBit(dst);
		}
	}

	// undo castling (move rook to its origin square in corner)
	if (piece_t == Piece::KING) {
		const bool short_castle = move.isShortCastle(),
				   long_castle = move.isLongCastle();

		if (short_castle) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst - 1, dst + 1);
			_occupied[_turn].moveBit(dst - 1, dst + 1);
		} 
		else if (long_castle) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst + 1, dst - 2);
			_occupied[_turn].moveBit(dst + 1, dst - 2);
		}

		_king_sq[_turn] = org;
	}

	_fullmove_count -= static_cast<int>(_turn);

	// recover old states that are irreversible
	_ep_square = prev_state.ep_sq;
	_halfmove_count = prev_state.halfmove_count;
	_castling_rights = prev_state.castling_rights;

	// TEMPORARY
	_zhash = prev_state.hash_key;
}

void Position::makeNull(IrreversibleState& state, nn::AccumulatorCache* accum_cache) {
	accum_cache->clearBuffers();

	_halfmove_count++;
	_fullmove_count += static_cast<uint16_t>(_turn);

	state.hash_key = _zhash;

	_turn = !_turn;
	_zhash ^= ZHashMasks->black_key;

	state.ep_sq = _ep_square;

	if (!_ep_square.isNull())
		_zhash ^= ZHashMasks->ep_file_keys[_ep_square.getFile()];

	_ep_square = Square::None;

	accum_cache->markDirty();
}

void Position::unmakeNull(const IrreversibleState& prev_state) {
	_turn = !_turn;
	
	_halfmove_count--;
	_fullmove_count -= static_cast<uint16_t>(_turn);

	_zhash = prev_state.hash_key;

	_ep_square = prev_state.ep_sq;
}

uint64_t Position::likelyZobristKeyAfterMove(Move32b& move) const {
	ASSERTNOLOG(!move.isNull());

	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const Piece::enumType piece_t = move.getPiece();

	uint64_t new_zhash = static_cast<uint64_t>(_zhash) ^ ZHashMasks->black_key;

	new_zhash ^= ZHashMasks->piece_keys[_turn][piece_t][org];
	new_zhash ^= ZHashMasks->piece_keys[_turn][piece_t][dst];

	if (move.isCapture() and !move.isEnPassant()) {
		const Piece::enumType captured = pieceOn(dst, !_turn);
		new_zhash ^= ZHashMasks->piece_keys[!_turn][captured][dst];
	}

	return new_zhash;
}

template <bool Root>
uint64_t Position::perft(unsigned depth) {
	if (depth == 0)
		return 1;

	Timer my_timer;

	if constexpr (Root)
		my_timer.go();

	uint64_t nodes = 0, child_nodes = 0;

	MoveList move_list;
	MoveGen::generatePseudoLegalMoves<MoveGen::ALL>(*this, move_list);

	IrreversibleState state = getIrreversibleState();

	for (size_t i = 0; i < move_list.count(); i++) {
		Move32b move = move_list.getMove(i);

		if (make(move)) {
			assert(_zhash == ZHash::generateOnFly(*this));

			child_nodes = perft<false>(depth - 1);
			nodes += child_nodes;

			if constexpr (Root) {
				move.print();
				std::cout << ": " << child_nodes << '\n';
				std::cout << std::flush;
			}
		}

		unmake(move, state);
	}

	if constexpr (Root) {
		time_ms_t duration_ms = my_timer.duration();
		duration_ms = duration_ms ? duration_ms : 1;

		std::cout << "total nodes: " << nodes << " (" << duration_ms / 1000.f << " seconds, " 
			<< nodes / duration_ms << "kN/sec.)" << '\n';
	}

	return nodes;
}

template uint64_t Position::perft<false>(unsigned depth);
template uint64_t Position::perft<true>(unsigned depth);

void Position::setGameStatesFromStr(const std::string fen, size_t i) {
    std::stringstream ss(fen.substr(i));
    std::string turn, 
				castling, 
				epstr;

    if (ss >> turn)
        _turn.fromChar(turn[0]);

    _castling_rights[WHITE].clear();
    _castling_rights[BLACK].clear();

    if (ss >> castling) {
        for (char c : castling) {
            switch (c) {
                case 'K': 
					_castling_rights[WHITE].setKingSide(true);  
					break;
                case 'Q': 
					_castling_rights[WHITE].setQueenSide(true); 
					break;
                case 'k': 
					_castling_rights[BLACK].setKingSide(true);  
					break;
                case 'q': 
					_castling_rights[BLACK].setQueenSide(true); 
					break;
                default: 
					break;
            }
        }
    }

    _ep_square = Square::None;

    if (ss >> epstr && epstr != "-") {
        if (epstr.length() >= 2)
            _ep_square = Square::fromChar(epstr[0], epstr[1]);
    }

	int probe_clock = 0;

    if (!(ss >> probe_clock))
        _halfmove_count = 0;
	else 
		_halfmove_count = probe_clock;

    if (!(ss >> probe_clock))
        _fullmove_count = 0;
	else 
		_fullmove_count = probe_clock;

    _zhash = ZHash::generateOnFly(*this);
}

_INLINE BitBoard xRayAttackers(BitBoard occ, Square sq, BitBoard bishopsQueens, BitBoard rooksQueens) {
	return ((bishopsQueens & attacks<Piece::BISHOP>(sq, occ))
		    | (rooksQueens & attacks<Piece::ROOK>(sq, occ))) & occ;
}

_INLINE BitBoard Position::getWeakestAttacker(BitBoard bb,
											  enumColor side,
											  Piece::uint_t& piece) const
{
	for (piece = Piece::PAWN; piece <= Piece::KING; piece++) {
		BitBoard mask = _piece_bb[side][piece] & bb;
		if (mask) return mask.oneBit();
	}
	return BitBoard(0_ui64);
}

static constexpr array1d<const int*, 6> SeePieceValue = {
	&SeePawnValue,
	&SeeKnightValue,
	&SeeBishopValue,
	&SeeRookValue,
	&SeeQueenValue,
	&SeeKingValue
};

template <bool ExactScore>
int Position::staticExchangeEval(Square org, 
								 Square sq, 
								 Piece::enumType target, 
								 Piece::enumType attacker) const 
{
	if (!ExactScore and *SeePieceValue[target] > *SeePieceValue[attacker])
		return 1;
	
	array1d<int, 32> gain;
	int i = 0;

	const BitBoard bishopsQueens = getBishops() | getQueens();
	const BitBoard rooksQueens = getRooks() | getQueens();
	const BitBoard xray = getPawns() | bishopsQueens | rooksQueens;
	const BitBoard targetbb = BitBoard(sq);

	BitBoard from = BitBoard(org);
	BitBoard occ = getOccupied() ^ from;
	enumColor side2move = getTurn();

	BitBoard attacks = (attacksTo(sq, !side2move, occ) ^ from) | attacksTo(sq, side2move, occ);

	Piece::uint_t vic = target;
	Piece::uint_t att = attacker;
	gain[i] = *SeePieceValue[vic];

	vic = att;
	if (vic == Piece::PAWN and targetbb & BitBoard::promorank(side2move)) {
		gain[i] += *SeePieceValue[Piece::QUEEN] - *SeePieceValue[Piece::PAWN];
		vic = Piece::QUEEN;
	}

	side2move = !side2move;
	from = getWeakestAttacker(attacks, side2move, att);

	while (from != 0_ui64) {
		i++;
		gain[i] = -gain[i - 1] + *SeePieceValue[vic];
		if constexpr (!ExactScore) {
			if (std::max(-gain[i - 1], gain[i]) < 0)
				break;
		}
		attacks ^= from;
		occ ^= from;
		if (from & xray) {
			attacks |= xRayAttackers(occ, sq, bishopsQueens, rooksQueens);
		}
		if (att == Piece::PAWN and targetbb & BitBoard::promorank(side2move)) {
			gain[i] += *SeePieceValue[Piece::QUEEN] - *SeePieceValue[Piece::PAWN];
			att = Piece::QUEEN;
		}
		side2move = !side2move;
		vic = att;
		from = getWeakestAttacker(attacks, side2move, att);
	}
	
	while (i > 0) {
		gain[i - 1] = -std::max(-gain[i - 1], gain[i]);
		i--;
	}

	return gain[0];
}

template <bool ExactScore>
int _StaticExchangeEval_unittest(const Position& pos, Square org, Square sq, 
	Piece::enumType target, Piece::enumType attacker) {
	return pos.staticExchangeEval<ExactScore>(org, sq, target, attacker);
}

template int Position::staticExchangeEval<false>(Square, Square, Piece::enumType, Piece::enumType) const;
template int Position::staticExchangeEval<true>(Square, Square, Piece::enumType, Piece::enumType) const;

template int _StaticExchangeEval_unittest<false>(const Position&, Square, Square, Piece::enumType, Piece::enumType);
template int _StaticExchangeEval_unittest<true>(const Position&, Square, Square, Piece::enumType, Piece::enumType);
