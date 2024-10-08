#include "Position.hpp"
#include "Move.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"

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
	clearPieces();

	int x = 0, y = 7;

	for (int i = 0; i < size(fen); i++) {
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
		_piece_bb[col][Piece::fromChar(col, c).toIndex()].setBit(in);
		++x;
	}
}

void Position::setStartingPos() {
	setByFEN(static_cast<std::string>(starting_fen));
}

void Position::print() const {
	std::cout << "     A   B   C   D   E   F   G   H";

	for (int h = 7; h >= 0; h--) {
		std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
			<< ' ' << h + 1 << " | ";

		for (int i = 8 * h; i < 8 * (h + 1); i++) {
			pieceOn(i).print();
			std::cout << " | ";
		}

		std::cout << h + 1;
	}

	std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
		<< "     A   B   C   D   E   F   G   H\n\n"
		<< "FEN States: ";

	_turn.print();
	std::cout << ' ';
	_castling_rights[WHITE].printByColor(WHITE);
	_castling_rights[BLACK].printByColor(BLACK);
	std::cout << ' ';
	_ep_square.print();
	std::cout << ' ' << static_cast<int>(_halfmove_count)
		<< ' ' << _fullmove_count << '\n';
}

bool Position::make(Move& move, IrreversibleState& state) {
	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  promotion = move.isPromotion();
	const Piece::enumType piece_t = move.getPerformerT();
	const int			  dir = _turn == WHITE ? 8 : -8;
	const bool			  pawn_push = piece_t == Piece::PAWN and !capture,
						  double_pawn_push = pawn_push and abs(org - dst) > 8;

	state.ep_sq = _ep_square;
	state.halfmove_count = _halfmove_count;
	state.castling_rights = _castling_rights;

	// TEMPORARY
	state.hash_key = _hashing._key;

	if (capture) {
		if (move.isEnPassant()) {
			assert(piece_t == Piece::PAWN);
			_piece_bb[!_turn][Piece::PAWN].popBit(dst - dir);
			_hashing._key ^= _hashing._piece_keys[!_turn][Piece::PAWN][dst - dir];
		}
		else {
			const Piece::enumType captured = pieceTypeOn(dst, !_turn);
			move.setCapturedT(captured);

			assert(captured != Piece::NONE);

			_piece_bb[!_turn][captured].popBit(dst);
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
		const Piece::enumType promo_piece_t = move.getPromoPieceT();
		assert(piece_t == Piece::PAWN and promo_piece_t != Piece::PAWN and promo_piece_t != Piece::KING);

		_piece_bb[_turn][piece_t].popBit(org);
		_piece_bb[_turn][promo_piece_t].setBit(dst);

		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][org];
		_hashing._key ^= _hashing._piece_keys[_turn][promo_piece_t][dst];
	}
	else { // if not a promotion - just move a piece on its own bitboard 
		_piece_bb[_turn][piece_t].moveBit(org, dst);

		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][org];
		_hashing._key ^= _hashing._piece_keys[_turn][piece_t][dst];
	}

	if (piece_t == Piece::KING) {
		if (move.isShortCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst + 1, dst - 1);

			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst + 1];
			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst - 1];
		}
		else if (move.isLongCastle()) {
			_piece_bb[_turn][Piece::ROOK].moveBit(dst - 2, dst + 1);

			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst - 2];
			_hashing._key ^= _hashing._piece_keys[_turn][Piece::ROOK][dst + 1];
		}

		_king_sq[_turn] = dst;
	}

	const bool legal = !isInCheck(_turn);

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

		_ep_square = Square::none;

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

void Position::unmake(Move move, const IrreversibleState& prev_state) {
	const Piece::enumType piece_t = move.getPerformerT();
	const Square		  org = move.getOrigin(),
						  dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  ep_capture = move.isEnPassant(),
						  promotion = move.isPromotion();

	_turn = !_turn;

	if (promotion) {
		const Piece::enumType promo_piece_t = move.getPromoPieceT();

		assert(piece_t == Piece::PAWN and promo_piece_t != Piece::PAWN and promo_piece_t != Piece::KING);
		_piece_bb[_turn][piece_t].setBit(org);
		_piece_bb[_turn][promo_piece_t].popBit(dst);
	}
	else // if not a promotion - just move a piece to origin square
		_piece_bb[_turn][piece_t].moveBit(dst, org);

	if (capture) {
		if (ep_capture) {
			const int dir = _turn == WHITE ? 8 : -8;

			assert(piece_t == Piece::PAWN);
			_piece_bb[!_turn][Piece::PAWN].setBit(dst - dir);
		}
		else {
			const Piece::enumType captured = move.getCapturedT();

			assert(captured != Piece::NONE);
			_piece_bb[!_turn][captured].setBit(dst);
		}
	}

	// undo castling (move rook to its origin square in corner)
	if (piece_t == Piece::KING) {
		const bool short_castle = move.isShortCastle(),
				   long_castle = move.isLongCastle();

		if (short_castle)
			_piece_bb[_turn][Piece::ROOK].moveBit(dst - 1, dst + 1);
		else if (long_castle)
			_piece_bb[_turn][Piece::ROOK].moveBit(dst + 1, dst - 2);

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

	_ep_square = Square::none;
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

	IrreversibleState state;

	for (int i = 0; i < move_list.count(); i++) {
		Move move = move_list.getMove(i);

		if (make(move, state)) {
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
		my_timer.stop();
		auto duration_ms = my_timer.duration();
		duration_ms = 0 ? 1 : duration_ms;

		std::cout << "total nodes: " << nodes << " (" << duration_ms / 1000.f << " seconds, " 
			<< nodes / duration_ms << "kN/sec.)" << '\n';
	}

	return nodes;
}

template uint64_t Position::perft<false>(unsigned depth);
template uint64_t Position::perft<true>(unsigned depth);

void Position::setGameStatesFromStr(const std::string fen, int i) {
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

	_ep_square = Square::none;

	if (fen[++i] != '-') {
		_ep_square = Square::fromChar(fen[i], fen[i + 1]);
	}

	i += 2;

	_halfmove_count = 0;
	for (; fen[i] != ' '; i++) {
		_halfmove_count *= 10;
		_halfmove_count += fen[i] - '0';
	}

	i++;

	_fullmove_count = 0;
	for (; i < size(fen) and fen[i] != ' '; i++) {
		_fullmove_count *= 10;
		_fullmove_count += fen[i] - '0';
	}

	_king_sq[WHITE] = getKingBySide(WHITE).bitScanForward();
	_king_sq[BLACK] = getKingBySide(BLACK).bitScanReverse();

	_hashing._key = _hashing.generateOnFly(*this);
}

/*
INLINE BitBoard Position::leastValuableAttacker_withMask(const Square sq, enumColor attacked, BitBoard occupied, 
	BitBoard mask, Piece::enumType& attacker) const {
	BitBoard bb;

	bb = pawnAttacks(sq, attacked) & getPawnsBySide(!attacked) & mask;
	if (bb) {
		attacker = Piece::PAWN;
		return bb;
	}

	bb = knightAttacks(sq) & getKnightsBySide(!attacked) & mask;
	if (bb) {
		attacker = Piece::KNIGHT;
		return bb;
	}

	bb = SlidersMagics::bishopAttacks(sq, occupied) & getBishopsBySide(!attacked) & mask;
	if (bb) {
		attacker = Piece::BISHOP;
		return bb;
	}

	bb = SlidersMagics::rookAttacks(sq, occupied) & getRooksBySide(!attacked) & mask;
	if (bb) {
		attacker = Piece::ROOK;
		return bb;
	}

	bb = SlidersMagics::queenAttacks(sq, occupied) & getQueensBySide(!attacked) & mask;
	attacker = Piece::QUEEN;
	return bb;
}


int Position::StaticExchangeEval(const Square sq) const {
	static constexpr std::array<int, 6> piece_value = {
		100, 300, 300, 500, 900, 10000
	};

	std::array<int, 33> gain;

	enumColor side = _turn;
	BitBoard processed = BitBoard::empty;

	const BitBoard occupied = getOccupied();
	
	int i = 0;
	gain[0] = 0;
	Piece::enumType victim = pieceTypeOn(sq, !side), weakest_att;
	BitBoard attackers = leastValuableAttacker_withMask(sq, !side, occupied, BitBoard::universe, weakest_att);
	
	while (attackers and victim != Piece::KING) {
		i++;
		gain[i] = -gain[i - 1] + piece_value[victim];

		victim = weakest_att;
		processed |= BitBoard(Square((_piece_bb[side][weakest_att] & ~processed).bitScanForward()));

		side = !side;
		attackers = leastValuableAttacker_withMask(sq, !side, occupied ^ processed, ~processed, weakest_att);
	}

	while (i > 1) {
		gain[i - 1] = -std::max(-gain[i - 1], gain[i]);
		i--;
	}

	return gain[1];
}
*/

int Position::StaticExchangeEval(const Square sq) const {
	static constexpr std::array<int, 6> piece_value = {
		100, 300, 300, 500, 900, 10000
	};

	static auto get_weakest_from = [this](BitBoard bb, enumColor side) _LAMBDA_FORCEINLINE {
		for (auto p : Piece::piece_list)
			if (_piece_bb[side][p] & bb) return p;
		return Piece::NONE;
	};

	std::array<int, 33> gain;

	enumColor side = _turn;
	BitBoard processed = BitBoard::empty;
	const BitBoard occupied = getOccupied();

	int i = 0;
	gain[0] = 0;
	Piece::enumType victim = pieceTypeOn(sq, !side);
	BitBoard attackers = attacksTo(sq, !side, occupied);

	while (attackers and victim != Piece::KING) {
		i++;
		gain[i] = -gain[i - 1] + piece_value[victim];

		victim = get_weakest_from(attackers, side);
		processed |= BitBoard(Square((_piece_bb[side][victim] & ~processed).bitScanForward()));

		side = !side;
		attackers = attacksTo(sq, !side, occupied ^ processed) & ~processed;
	}

	while (i > 1) {
		gain[i - 1] = -std::max(-gain[i - 1], gain[i]);
		i--;
	}

	return gain[1];
}