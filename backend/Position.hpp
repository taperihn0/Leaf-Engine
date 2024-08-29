#include "Common.hpp"
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
	Square() = default;
	constexpr Square(uint8_t cpy)
		: _sq(cpy) {}

	INLINE constexpr Square operator=(uint8_t sq) {
		return _sq = sq;
	}

	void fromChar(char rank, char file);

	static constexpr uint8_t none = -1Ui8;
private:
	uint8_t _sq;
};

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

class Position {
public:
	Position() = default;

	// assuming given FEN is valid FEN position
	void setByFEN(const std::string fen);

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
};