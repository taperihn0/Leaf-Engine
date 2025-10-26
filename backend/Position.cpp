#include "Position.hpp"
#include "Move.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "Search.hpp"

CastlingRights::CastlingRights(bool kinit, bool qinit) 
	: _kingside(kinit), _queenside(qinit) {}

void CastlingRights::printByColor(enumColor col_type) const {
	std::string msg;
	if (_kingside) msg += col_type == BLACK ? 'k' : 'K';
	if (_queenside) msg += col_type == BLACK ? 'q' : 'Q';
	std::cout << msg;
}

Position::Position() { setStartingPos(); }

Position::Position(const std::string init_fen) { setByFEN(init_fen); }

Position::Position(const std::string_view init_fen) { setByFEN(static_cast<std::string>(init_fen)); }

void Position::setByFEN(const std::string fen) {
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

void Position::print() const {
	std::cout << "     A   B   C   D   E   F   G   H";

	for (int h = 7; h >= 0; h--) {
		std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
			<< ' ' << h + 1 << " | ";

		for (int i = 8 * h; i < 8 * (h + 1); i++) {
			fullPieceOn(i).print();
			std::cout << " | ";
		}

		std::cout << h + 1;
	}

	std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
		<< "     A   B   C   D   E   F   G   H\n\n"
		<< "FEN States: ";

	_turn.print();
	std::cout << ' ';

	if (_castling_rights[WHITE].isAnyPossible())
		_castling_rights[WHITE].printByColor(WHITE);
	if (_castling_rights[BLACK].isAnyPossible())
		_castling_rights[BLACK].printByColor(BLACK);

	if (!_castling_rights[WHITE].isAnyPossible() and 
		!_castling_rights[BLACK].isAnyPossible())
		std::cout << '-';

	std::cout << ' ';
	_ep_square.print();
	std::cout << ' ' << static_cast<int>(_halfmove_count)
		<< ' ' << _fullmove_count << '\n';
}

bool Position::make(Move32b& move) {
	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  promotion = move.isPromotion();
	const Piece::enumType piece_t = move.getPiece();
	const int			  dir = _turn == WHITE ? 8 : -8;
	const bool			  pawn_push = piece_t == Piece::PAWN and !capture,
						  double_pawn_push = pawn_push and (org - dst > 8 or dst - org > 8);

	if (capture) {
		if (move.isEnPassant()) {
			assert(piece_t == Piece::PAWN);
			_piece_bb[!_turn][Piece::PAWN].popBit(dst - dir);
			_occupied[!_turn].popBit(dst - dir);
			_hashing._key ^= _hashing._piece_keys[!_turn][Piece::PAWN][dst - dir];
		}
		else {
			const Piece::enumType captured = pieceOn(dst, !_turn);
			move.setCaptured(captured);

			assert(captured != Piece::NONE and captured != Piece::KING);

			_piece_bb[!_turn][captured].popBit(dst);
			_occupied[!_turn].popBit(dst);
			_hashing._key ^= _hashing._piece_keys[!_turn][captured][dst];

			const Square RightCornerOpponent = _turn == BLACK ? Square::h1 : Square::h8,
				LeftCornerOpponent = _turn == BLACK ? Square::a1 : Square::a8;

			if (_castling_rights[!_turn].isShortPossible() and dst == RightCornerOpponent) {
				_hashing._key ^= _hashing._short_castle_keys[!_turn];
				_castling_rights[!_turn].setKingSide(false);
			}
			else if (_castling_rights[!_turn].isLongPossible() and dst == LeftCornerOpponent) {
				_hashing._key ^= _hashing._long_castle_keys[!_turn];
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

		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][org];
		_hashing._key ^= _hashing._piece_keys[_turn][promo_piece_t][dst];
	}
	else { // if not a promotion - just move a piece on its own bitboard 
		_piece_bb[_turn][piece_t].moveBit(org, dst);
		_occupied[_turn].moveBit(org, dst);

		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][org];
		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][dst];
	}

	if (piece_t == Piece::KING) {
		if (move.isShortCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst + 1, dst - 1);
			_occupied[_turn].moveBit(dst + 1, dst - 1);

			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst + 1];
			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst - 1];
		}
		else if (move.isLongCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst - 2, dst + 1);
			_occupied[_turn].moveBit(dst - 2, dst + 1);

			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst - 2];
			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst + 1];
		}

		_king_sq[_turn] = dst;
	}

	const bool legal = !isInCheck(_turn);
	move.setLegalMoved(legal);

	// Just leave castling flags untouched since the move is pseudo-legal.
	// It will be ignored anyway in the search.
	if (legal) {
		const Square RightCorner = _turn == WHITE ? Square::h1 : Square::h8,
			LeftCorner = _turn == WHITE ? Square::a1 : Square::a8;

		if (_castling_rights[_turn].isShortPossible() and (piece_t == Piece::KING or getRooksBySide(_turn).isEmptySq(RightCorner))) {
			_hashing._key ^= _hashing._short_castle_keys[_turn];
			_castling_rights[_turn].setKingSide(false);
		}

		if (_castling_rights[_turn].isLongPossible() and (piece_t == Piece::KING or getRooksBySide(_turn).isEmptySq(LeftCorner))) {
			_hashing._key ^= _hashing._long_castle_keys[_turn];
			_castling_rights[_turn].setQueenSide(false);
		}

		// reset old en passant square state
		if (_ep_square.isNotNull())
			_hashing._key ^= _hashing._ep_file_keys[_ep_square.getFile()];

		_ep_square = Square::None;

		if (double_pawn_push) {
			_ep_square = dst - dir;
			_hashing._key ^= _hashing._ep_file_keys[_ep_square.getFile()];
		}

		_hashing._key ^= _hashing._black_key;

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
	_hashing._key = prev_state.hash_key;
}

void Position::makeNull(IrreversibleState& state) {
	_halfmove_count++;
	_fullmove_count += static_cast<uint16_t>(_turn);

	state.hash_key = _hashing._key;

	_turn = !_turn;
	_hashing._key ^= _hashing._black_key;

	state.ep_sq = _ep_square;

	if (_ep_square.isNotNull())
		_hashing._key ^= _hashing._ep_file_keys[_ep_square.getFile()];

	_ep_square = Square::None;
}

void Position::unmakeNull(const IrreversibleState& prev_state) {
	_turn = !_turn;
	
	_halfmove_count--;
	_fullmove_count -= static_cast<uint16_t>(_turn);

	_hashing._key = prev_state.hash_key;

	_ep_square = prev_state.ep_sq;
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
			assert(_hashing._key == _hashing.generateOnFly(*this));

			child_nodes = perft<false>(depth - 1);
			nodes += child_nodes;

			if constexpr (Root) {
				move.print();
				std::cout << ": " << child_nodes << '\n';
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
	_turn.fromChar(fen[i]);

	i += 2;
	_castling_rights[WHITE].clear(), _castling_rights[BLACK].clear();

	for (; fen[i] != ' '; i++) {
		switch (fen[i]) {
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

	_ep_square = Square::None;

	if (fen[++i] != '-') {
		_ep_square = Square::fromChar(fen[i], fen[i + 1]);
	}

	i += 2;

	_halfmove_count = 0;
	for (; i < size(fen) and fen[i] != ' '; i++) {
		_halfmove_count *= 10;
		_halfmove_count += fen[i] - '0';
	}

	i++;

	_fullmove_count = 0;
	for (; i < size(fen) and fen[i] != ' '; i++) {
		_fullmove_count *= 10;
		_fullmove_count += fen[i] - '0';
	}

	_hashing._key = _hashing.generateOnFly(*this);
}

INLINE BitBoard xRayAttackers(BitBoard occ, Square sq, BitBoard bishopsQueens, BitBoard rooksQueens) {
	return ((bishopsQueens & attacks<Piece::BISHOP>(sq, occ))
		    | (rooksQueens & attacks<Piece::ROOK>(sq, occ))) & occ;
}

static constexpr std::array<int, 7> SeePieceValue = {
	100, 300, 300, 500, 900, 10000, 0
};

template <bool ExactScore>
int Position::StaticExchangeEval(Square org, Square sq, Piece::enumType target, Piece::enumType attacker) const {
	static auto get_weakest_from = [this](BitBoard bb, enumColor side, Piece::uint_t& piece) _LAMBDA_FORCEINLINE {
		for (piece = Piece::PAWN; piece <= Piece::KING; piece++) {
			BitBoard mask = _piece_bb[side][piece] & bb;
			if (mask) return mask.oneBit();
		}
		return BitBoard(0_ui64);
	};

	if (!ExactScore and
		SeePieceValue[target] > SeePieceValue[attacker])
	{
		return 1;
	}
	
	int gain[32];
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
	gain[i] = SeePieceValue[vic];

	vic = att;
	if (vic == Piece::PAWN and targetbb & BitBoard::promorank(side2move)) {
		gain[i] += SeePieceValue[Piece::QUEEN] - SeePieceValue[Piece::PAWN];
		vic = Piece::QUEEN;
	}

	side2move = !side2move;
	from = get_weakest_from(attacks, side2move, att);

	while (from != 0_ui64) {
		i++;
		gain[i] = -gain[i - 1] + SeePieceValue[vic];
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
			gain[i] += SeePieceValue[Piece::QUEEN] - SeePieceValue[Piece::PAWN];
			att = Piece::QUEEN;
		}
		side2move = !side2move;
		vic = att;
		from = get_weakest_from(attacks, side2move, att);
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
	return pos.StaticExchangeEval<ExactScore>(org, sq, target, attacker);
}

template int Position::StaticExchangeEval<false>(Square, Square, Piece::enumType, Piece::enumType) const;
template int Position::StaticExchangeEval<true>(Square, Square, Piece::enumType, Piece::enumType) const;

template int _StaticExchangeEval_unittest<false>(const Position&, Square, Square, Piece::enumType, Piece::enumType);
template int _StaticExchangeEval_unittest<true>(const Position&, Square, Square, Piece::enumType, Piece::enumType);
