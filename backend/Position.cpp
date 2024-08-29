#include "Position.hpp"

#include <string_view>

void Square::fromChar(char rank, char file) {
	_sq = (rank - 'a') + (file - '1') * 8;
}

CastlingRights::CastlingRights(bool kinit, bool qinit) 
: kingside(kinit), queenside(qinit) {}

void CastlingRights::printByColor(enumColor col_type) {
	std::string msg;
	if (kingside) msg += col_type == BLACK ? 'k' : 'K';
	if (queenside) msg += col_type == BLACK ? 'q' : 'Q';
	std::cout << msg;
}

void Position::setByFEN(const std::string fen) {
	clearPieces();

	static std::array<std::string_view, 2> piece_str_by_side = { "PNBRQK", "pnbrqk" };

	int x = 0, y = 7;

	for (int i = 0; i < size(fen); i++) {
		const char c = fen[i];

		if (isdigit(c)) {
			x += c - '0';
			continue;
		}
		else if (c == ' ') {
			setGameStates(fen, i + 1);
			break;
		}

		int in = y * 8 + x;
		if (c == '/') {
			y--, x = 0;
			continue;
		}

		const bool side = islower(c);
		const auto piece_t = piece_str_by_side[side].find_first_of(c);

		_piece_bb[side][piece_t].setBit(in);
		++x;
	}
}

void Position::setGameStates(const std::string fen, int i) {
	_turn = fen[i] == 'w' ? WHITE : BLACK;

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