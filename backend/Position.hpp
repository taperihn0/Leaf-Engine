#pragma once

#include "BitBoard.hpp"

enum enumPiece : uint8_t {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING
};

enum enumColor : bool {
	WHITE, BLACK
};

INLINE constexpr enumColor operator!(enumColor opp) {
	return static_cast<enumColor>(!static_cast<bool>(opp));
}

class Turn {
public:
	Turn() = default;
	constexpr Turn(enumColor c)
		: _col(c) {}

	INLINE constexpr Turn operator=(enumColor c) {
		return _col = c;
	}

	INLINE constexpr Turn operator!() const {
		return !_col;
	}

	enumColor toColor() {
		return _col;
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

	INLINE bool getKingSide() noexcept {
		return _kingside;
	}

	INLINE bool getQueenSide() noexcept {
		return _queenside;
	}

	INLINE void setKingSide(bool flag) noexcept {
		_kingside = flag;
	}

	INLINE void setQueenSide(bool flag) noexcept {
		_queenside = flag;
	}

	INLINE void clear() noexcept {
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

	void print();

	INLINE BitBoard getPawnsBySide(enumColor col_type) {
		return _piece_bb[col_type][PAWN];
	}

	INLINE BitBoard getKnightsBySide(enumColor col_type) {
		return _piece_bb[col_type][KNIGHT];
	}

	INLINE BitBoard getBishopsBySide(enumColor col_type) {
		return _piece_bb[col_type][BISHOP];
	}

	INLINE BitBoard getRooksBySide(enumColor col_type) {
		return _piece_bb[col_type][ROOK];
	}

	INLINE BitBoard getQueensBySide(enumColor col_type) {
		return _piece_bb[col_type][QUEEN];
	}

	INLINE BitBoard getKingBySide(enumColor col_type) {
		return _piece_bb[col_type][KING];
	}

	INLINE BitBoard getByColor(enumColor col_type) {
		return _piece_bb[col_type][PAWN]
			| _piece_bb[col_type][KNIGHT]
			| _piece_bb[col_type][BISHOP]
			| _piece_bb[col_type][ROOK]
			| _piece_bb[col_type][QUEEN]
			| _piece_bb[col_type][KING];
	}

	INLINE BitBoard getWhites() {
		return getByColor(WHITE);
	}

	INLINE BitBoard getBlacks() {
		return getByColor(BLACK);
	}

	INLINE BitBoard getOccupied() {
		return getWhites() | getBlacks();
	}

	INLINE BitBoard getOppositePieces() {
		return getByColor(!_turn.toColor());
	}

	INLINE BitBoard getEmpties() {
		return getOccupied() ^ BitBoard::universe;
	}

	INLINE Turn getOppositeTurn() {
		return !_turn;
	}

	INLINE CastlingRights getCastlingByColor(enumColor col_type) {
		return _castling_rights[col_type];
	}

	INLINE void setTurn(enumColor col_to_move) {
		_turn = col_to_move;
	}

	static constexpr std::string_view starting_fen 
		= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	static constexpr std::string_view empty_fen
		= "/////// w - - 0 1";

private:
	INLINE void clearPieces() {
		for (enumColor col : { WHITE, BLACK })
			_piece_bb[col].fill(BitBoard::empty);
	}

	void setGameStates(const std::string fen, int i);

	std::array<std::array<BitBoard, 6>, 2> _piece_bb;
	Turn _turn;

	std::array<CastlingRights, 2> _castling_rights;

	Square _ep_square;

	uint8_t _halfmove_count;
	uint16_t _fullmove_count;

	std::string _cur_fen;

	static constexpr std::array<std::string_view, 2> _piece_str_by_side = { "PNBRQK", "pnbrqk" };
};