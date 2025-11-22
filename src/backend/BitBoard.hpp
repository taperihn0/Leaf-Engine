#pragma once

#include "Common.hpp"
#include "Square.hpp"
#include "Color.hpp"

// Distict type to wrap raw bitboard type
class BitBoard {
public:
	BitBoard() = default;
	constexpr BitBoard(const BitBoard&) = default;
	constexpr BitBoard(BitBoard&&) = default;

	INLINE constexpr BitBoard(uint64_t raw_init)
		: _board(raw_init) {}

	INLINE constexpr BitBoard(Square sq)
		: _board(1_ui64 << sq) {}

	INLINE constexpr BitBoard(Square::enumSquare sq)
		: _board(1_ui64 << sq) {}

	INLINE constexpr operator uint64_t() const {
		return _board;
	}

	INLINE constexpr BitBoard operator=(const BitBoard& cpy) {
		return _board = cpy._board;
	}

	INLINE constexpr BitBoard operator|=(BitBoard bb) {
		return _board |= bb._board;
	}

	INLINE constexpr BitBoard operator&=(BitBoard bb) {
		return _board &= bb._board;
	}

	INLINE constexpr BitBoard operator^=(BitBoard bb) {
		return _board ^= bb._board;
	}

	INLINE constexpr BitBoard operator>>=(int shift) {
		return _board >>= shift;
	}

	INLINE constexpr BitBoard operator<<=(int shift) {
		return _board <<= shift;
	}

	INLINE constexpr BitBoard operator|(BitBoard bb) const {
		return _board | bb._board;
	}

	INLINE constexpr BitBoard operator^(BitBoard bb) const {
		return _board ^ bb._board;
	}

	INLINE constexpr BitBoard operator^(uint64_t raw) const {
		return _board ^ raw;
	}

	INLINE constexpr BitBoard operator&(BitBoard bb) const {
		return _board & bb._board;
	}

	INLINE constexpr BitBoard operator&(uint64_t raw) const {
		return _board & raw;
	}

	INLINE constexpr BitBoard operator>>(int shift) const {
		return _board >> shift;
	}

	INLINE constexpr BitBoard operator<<(int shift) const {
		return _board << shift;
	}

	INLINE constexpr BitBoard operator*(BitBoard bb) const {
		return _board * bb._board;
	}

	INLINE constexpr BitBoard operator~() const {
		return ~_board;
	}

	INLINE constexpr BitBoard operator-() const {
		return static_cast<uint64_t>(-_board);
	}

	template <int Shift>
	INLINE BitBoard genShift() const {
		if constexpr (Shift < 0) return _board >> (-Shift);
		return _board << Shift;
	}

	INLINE BitBoard genShift(int shift) const {
		if (shift < 0) return _board >> (-shift);
		return _board << shift;
	}

	template <int Shift>
	INLINE BitBoard pawnsAttack() const {
		static_assert(Shift == 7 or Shift == -7 or Shift == 9 or Shift == -9);
		static constexpr BitBoard ExclFile = Shift == 7 or Shift == -9 ? Not_H_File : Not_A_File;
		return genShift<Shift>() & ExclFile;
	}

#if defined(DEBUG)
	void print() const;
#endif

	void set(uint64_t bb);

	int popCount() const;
	int bitScanForward() const;
	int bitScanReverse() const;

	// bit scan forward but with LS1B reset
	INLINE int dropForward() {
		const int ls1b = bitScanForward();
		_board &= _board - 1;
		return ls1b;
	}

	INLINE void popBit(Square sq) {
		assert(sq.isValid() and sq.isNotNull());
		_board &= ~(1_ui64 << sq);
	}

	INLINE void setBit(int shift) {
		assert(shift < 64);
		_board |= (1_ui64 << shift);
	}

	INLINE bool getBit(int shift) const {
		assert(shift < 64);
		return _board & (1_ui64 << shift);
	}

	INLINE bool isEmptySq(Square sq) const {
		return !getBit(sq);
	}

	INLINE bool isOccupiedSq(Square sq) const {
		return getBit(sq);
	}

	INLINE void moveBit(Square origin, Square target) {
		assert(getBit(origin));
		popBit(origin);
		setBit(target);
	}

	INLINE BitBoard oneBit() const {
		return _board & -_board;
	}

	template <int Rank>
	static INLINE constexpr BitBoard rank() {
		static_assert(1 <= Rank and Rank <= 8, "Invalid rank");
		return BitBoard(0xff_ui64 << ((Rank - 1) * 8));
	}

	static INLINE constexpr BitBoard rank(int rank) {
		ASSERT(1 <= rank and rank <= 8, "Invalid rank");
		return BitBoard(0xff_ui64 << ((rank - 1) * 8));
	}

	static INLINE constexpr BitBoard promorank(enumColor side) {
		return side == WHITE ? rank<8>() : rank<1>();
	}

	template <File TFile>
	static INLINE constexpr BitBoard file() {
		return BitBoard(A_File << static_cast<int>(TFile));
	}

	static INLINE constexpr BitBoard file(int file) {
		ASSERT(1 <= file and file <= 8, "Invalid file");
		return BitBoard(A_File << file);
	}

	// crucial uint64_t constants
	static constexpr uint64_t Universe    = 0xffffffffffffffff_ui64,
							  A_File	  = 0x0101010101010101_ui64,
							  B_File	  = 0x0202020202020202_ui64,
							  G_File	  = 0x4040404040404040_ui64,
							  H_File	  = 0x8080808080808080_ui64,
							  Not_A_File  = ~A_File,
						      Not_B_File  = ~B_File,
						      Not_G_File  = ~G_File,
						      Not_H_File  = ~H_File,
						      Not_AB_File = Not_A_File & Not_B_File,
						      Not_GH_File = Not_G_File & Not_H_File;
private:
	uint64_t _board;
};

static_assert(sizeof(BitBoard) == 8);

// Rectangular lookup for in-between routines
struct RectangularTable {
	using tab64x64_t = std::array<std::array<BitBoard, 64>, 64>;

	RectangularTable() { init(); }

	BitBoard inBetweenOnFly(Square org, Square dst);
	void init();

	tab64x64_t t64;
};

inline const RectangularTable rectangular;

// General setwise operations on BitBoard wrapper class *

namespace {

// one step only and shifting routines *

INLINE BitBoard nortOne(BitBoard bb) {
	return bb << 8;
}

INLINE BitBoard soutOne(BitBoard bb) {
	return bb >> 8;
}

INLINE BitBoard westOne(BitBoard bb) {
	return (bb >> 1) & BitBoard::Not_H_File;
}

INLINE BitBoard eastOne(BitBoard bb) {
	return (bb << 1) & BitBoard::Not_A_File;
}

INLINE BitBoard noEaOne(BitBoard bb) {
	return (bb << 9) & BitBoard::Not_A_File;
}

INLINE BitBoard soEaOne(BitBoard bb) {
	return (bb >> 7) & BitBoard::Not_A_File;
}

INLINE BitBoard soWeOne(BitBoard bb) {
	return (bb >> 9) & BitBoard::Not_H_File;
}

INLINE BitBoard noWeOne(BitBoard bb) {
	return (bb << 7) & BitBoard::Not_H_File;
}

INLINE BitBoard noNoEa(BitBoard bb) {
	return (bb << 17) & BitBoard::Not_A_File;
}

INLINE BitBoard noEaEa(BitBoard bb) {
	return (bb << 10) & BitBoard::Not_AB_File;
}

INLINE BitBoard soEaEa(BitBoard bb) {
	return (bb >> 6) & BitBoard::Not_AB_File;
}

INLINE BitBoard soSoEa(BitBoard bb) {
	return (bb >> 15) & BitBoard::Not_A_File;
}

INLINE BitBoard soSoWe(BitBoard bb) {
	return (bb >> 17) & BitBoard::Not_H_File;
}

INLINE BitBoard soWeWe(BitBoard bb) {
	return (bb >> 10) & BitBoard::Not_GH_File;
}

INLINE BitBoard noWeWe(BitBoard bb) {
	return (bb << 6) & BitBoard::Not_GH_File;
}

INLINE BitBoard noNoWe(BitBoard bb) {
	return (bb << 15) & BitBoard::Not_H_File;
}

INLINE BitBoard inBetween(Square org, Square dst) {
	assert(org.isValid() and org.isNotNull() and dst.isValid() and dst.isNotNull());
	return rectangular.t64[org][dst];
}

} // namespace