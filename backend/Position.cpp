#include "Position.hpp"

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
			setGameStates(fen, i + 1);
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

			Piece piece = Piece::enumType::NONE;

			for (enumColor col_t : { WHITE, BLACK }) {
				for (Piece::enumType piece_t : Piece::piece_list) {
					if (!_piece_bb[col_t][piece_t].getBit(i))
						continue;

					piece.set(col_t, piece_t);
					break;
				}
			}

			piece.print();
			std::cout << " | ";
		}

		std::cout << h + 1;
	}

	std::cout << "\n   +---+---+---+---+---+---+---+---+\n"
		<< "     A   B   C   D   E   F   G   H\n\n"
		<< "FEN: " << _cur_fen << '\n';
}

void Position::setGameStates(const std::string fen, int i) {
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