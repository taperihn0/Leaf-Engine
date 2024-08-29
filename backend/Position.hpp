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

class Square {
public:
	// little endian file-rank mapping
	enum enumSquare {
		a1, a2, a3, a4, a5, a6, a7, a8,
		b1, b2, b3, b4, b5, b6, b7, b8,
		c1, c2, c3, c4, c5, c6, c7, c8,
		d1, d2, d3, d4, d5, d6, d7, d8,
		e1, e2, e3, e4, e5, e6, e7, e8,
		f1, f2, f3, f4, f5, f6, f7, f8,
		g1, g2, g3, g4, g5, g6, g7, g8,
		h1, h2, h3, h4, h5, h6, h7, h8
	};

	Square() = default;
	constexpr Square(uint8_t cpy)
		: _sq(cpy) {}

	INLINE constexpr Square operator=(uint8_t sq) {
		return _sq = sq;
	}

	INLINE constexpr operator int() {
		return _sq;
	}

	void fromChar(char rank, char file) {
		_sq = (rank - 'a') + (file - '1') * 8;
	}

	void print() {
		std::cout << "abcdefgh"[_sq / 8] << (_sq % 8);
	}

	static constexpr uint8_t none = -1Ui8;
private:
	uint8_t _sq;
};

// wrapper around castling rights for single player
class CastlingRights {
public:
	CastlingRights() = default;
	CastlingRights(bool kinit, bool qinit);

	INLINE bool getKingSide() noexcept {
		return kingside;
	}

	INLINE bool getQueenSide() noexcept {
		return queenside;
	}

	INLINE void setKingSide(bool flag) noexcept {
		kingside = flag;
	}

	INLINE void setQueenSide(bool flag) noexcept {
		queenside = flag;
	}

	INLINE void clear() noexcept {
		kingside = false, queenside = false;
	}

	void printByColor(enumColor col_type);
private:
	bool kingside, queenside;
};

// internal board state, including piece distribution 
// and game flags like castling
class Position {
public:
	Position();
	Position(const std::string init_fen);

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
		return getByColor(!_turn);
	}

	INLINE BitBoard getEmpties() {
		return getOccupied() ^ BitBoard::universe;
	}

	INLINE enumColor getOppositeTurn() {
		return !_turn;
	}

	INLINE CastlingRights getCastlingByColor(enumColor col_type) {
		return _castling_rights[col_type];
	}

	INLINE void setTurn(enumColor col_to_move) {
		_turn = col_to_move;
	}

	static constexpr std::string_view starting_pos 
		= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
	INLINE void clearPieces() {
		for (enumColor col : { WHITE, BLACK })
			_piece_bb[col].fill(BitBoard::empty);
	}

	void setGameStates(const std::string fen, int i);

	std::array<std::array<BitBoard, 6>, 2> _piece_bb;
	enumColor _turn;

	std::array<CastlingRights, 2> _castling_rights;

	Square _ep_square;

	uint8_t _halfmove_count;
	uint16_t _fullmove_count;

	std::string cur_fen;

	static constexpr std::array<std::string_view, 2> piece_str_by_side = { "PNBRQK", "pnbrqk" };
};