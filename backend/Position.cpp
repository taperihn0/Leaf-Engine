#include "Position.hpp"

#include <string_view>

CastlingRights::CastlingRights(bool kinit, bool qinit) 
	: kingside(kinit), queenside(qinit) {}

void CastlingRights::printByColor(enumColor col_type) {
	std::string msg;
	if (kingside) msg += col_type == BLACK ? 'k' : 'K';
	if (queenside) msg += col_type == BLACK ? 'q' : 'Q';
	std::cout << msg;
}

Position::Position() { setStartingPos(); }

Position::Position(const std::string init_fen) { setByFEN(init_fen); }

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

	cur_fen = fen;
}

void Position::setStartingPos() {
	setByFEN(static_cast<std::string>(starting_pos));
}

void Position::print() {
	std::cout << "     A   B   C   D   E   F   G   H";

	for (int h = 7; h >= 0; h--) {
		std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
			<< ' ' << h + 1 << " | ";

		for (int i = 8 * h; i < 8 * (h + 1); i++) {

			char sqpiece_char = ' ';

			for (enumColor col_t : { WHITE, BLACK }) {
				for (enumPiece piece_t : { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING }) {
					if (!_piece_bb[col_t][piece_t].getBit(i))
						continue;

					sqpiece_char = piece_str_by_side[col_t][piece_t];
					break;
				}
			}

			std::cout << sqpiece_char << " | ";
		}

		std::cout << h + 1;
	}

	std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
		<< "     A   B   C   D   E   F   G   H\n\n"
		<< "FEN: " << cur_fen << '\n';
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