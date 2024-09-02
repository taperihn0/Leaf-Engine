#include "Position.hpp"
#include "Move.hpp"

CastlingRights::CastlingRights(bool kinit, bool qinit) 
	: _kingside(kinit), _queenside(qinit) {}

void CastlingRights::printByColor(enumColor col_type) {
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

	_cur_fen = fen;
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
			Piece(pieceTypeOn(i)).print();
			std::cout << " | ";
		}

		std::cout << h + 1;
	}

	std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
		<< "     A   B   C   D   E   F   G   H\n\n"
		<< "FEN: " << _cur_fen << '\n';
}

void Position::make(Move& move) {
	const Square org = move.getOrigin(),
				 dst = move.getTarget();
	const bool			  capture = move.isCapture(),
						  ep_capture = move.isEnPassant(),
						  promotion = move.isPromotion();
	const Piece::enumType piece_t = move.getPerformerT(),
						  captured = ep_capture ? Piece::PAWN : pieceTypeOn(dst),
						  promo_piece_t = move.getPromoPieceT();
	const bool			  pawn_push = piece_t == Piece::PAWN and !capture and !ep_capture;
	const int			  dir = _turn == WHITE ? 8 : -8;

	if (promotion) {
		assert(piece_t == Piece::PAWN);
		assert(promo_piece_t != Piece::PAWN and promo_piece_t != Piece::KING);
		_piece_bb[_turn][piece_t].popBit(org);
		_piece_bb[_turn][promo_piece_t].setBit(dst);
	}
	else {
		_piece_bb[_turn][piece_t].moveBit(org, dst);
	}

	if (ep_capture) {
		assert(piece_t == Piece::PAWN and captured == Piece::PAWN);
		_piece_bb[!_turn][captured].popBit(dst - dir);
	} 
	else if (capture) {
		_piece_bb[!_turn][captured].popBit(dst);
	}

	_halfmove_count = capture or ep_capture or pawn_push ? 0 : _halfmove_count + 1;
	_fullmove_count += static_cast<int>(_turn);
	_turn = !_turn;
}

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
		_ep_square.fromChar(fen[i], fen[i + 1]);
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
}