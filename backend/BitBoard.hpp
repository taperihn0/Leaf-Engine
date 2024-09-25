#pragma once

#include "Common.hpp"
#include "Square.hpp"

#include <cstdint>

template <typename T>
constexpr inline uint64_t U64(T val) {
	static_assert(std::is_integral<T>(), "U64 casting restricted to integer types");
	return static_cast<uint64_t>(val);
}

// Distict type to wrap raw bitboard type
class BitBoard {
public:
	BitBoard() = default;
	constexpr BitBoard(const BitBoard&) = default;
	constexpr BitBoard(BitBoard&&) = default;

	inline constexpr BitBoard(uint64_t raw_init)
		: _board(raw_init) {}

	inline constexpr BitBoard(Square sq)
		: _board(1Ui64 << sq) {}

	inline constexpr BitBoard(Square::enumSquare sq)
		: _board(1Ui64 << sq) {}

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

	template <int Shift>
	INLINE BitBoard genShift() const {
		if constexpr (Shift < 0) return _board >> (-Shift);
		return _board << Shift;
	}

	template <int Shift>
	INLINE BitBoard pawnsAttack() const {
		static_assert(Shift == 7 or Shift == -7 or Shift == 9 or Shift == -9);
		static constexpr BitBoard ExclFile = Shift == 7 or Shift == -9 ? not_h_file : not_a_file;
		return genShift<Shift>() & ExclFile;
	}

	// debug-purpose method
	void printRaw() const;
	
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
		_board &= ~(1Ui64 << sq);
	}

	INLINE void setBit(int shift) {
		assert(shift < 64);
		_board |= (1Ui64 << shift);
	}

	INLINE bool getBit(int shift) const {
		assert(shift < 64);
		return _board & (1Ui64 << shift);
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

	template <int Rank>
	static INLINE constexpr BitBoard rank() {
		static_assert(1 <= Rank and Rank <= 8, "Invalid rank");
		return BitBoard(0xffUi64 << ((Rank - 1) * 8));
	}

	template <File File_>
	static INLINE constexpr BitBoard file() {
		return BitBoard(a_file << static_cast<int>(File_));
	}

	// crucial uint64_t constants
	static constexpr uint64_t universe = 0xffffffffffffffffUi64,
							  empty = 0Ui64,
							  a_file = 0x0101010101010101Ui64,
							  b_file = 0x0202020202020202Ui64,
							  g_file = 0x4040404040404040Ui64,
							  h_file = 0x8080808080808080Ui64,
							  not_a_file = ~a_file,
						      not_b_file = ~b_file,
						      not_g_file = ~g_file,
						      not_h_file = ~h_file,
						      not_ab_file = not_a_file & not_b_file,
						      not_gh_file = not_g_file & not_h_file;
private:
	uint64_t _board;
};

// Rectangular lookup for in-between routines
struct RectangularTable {
	RectangularTable() { init(); }

	BitBoard inBetweenOnFly(Square org, Square dst);
	void init();

	std::array<std::array<BitBoard, 64>, 64> table;
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
		return (bb >> 1) & BitBoard::not_h_file;
	}

	INLINE BitBoard eastOne(BitBoard bb) {
		return (bb << 1) & BitBoard::not_a_file;
	}

	INLINE BitBoard noEaOne(BitBoard bb) {
		return (bb << 9) & BitBoard::not_a_file;
	}

	INLINE BitBoard soEaOne(BitBoard bb) {
		return (bb >> 7) & BitBoard::not_a_file;
	}

	INLINE BitBoard soWeOne(BitBoard bb) {
		return (bb >> 9) & BitBoard::not_h_file;
	}

	INLINE BitBoard noWeOne(BitBoard bb) {
		return (bb << 7) & BitBoard::not_h_file;
	}

	INLINE BitBoard noNoEa(BitBoard bb) {
		return (bb << 17) & BitBoard::not_a_file;
	}

	INLINE BitBoard noEaEa(BitBoard bb) {
		return (bb << 10) & BitBoard::not_ab_file;
	}

	INLINE BitBoard soEaEa(BitBoard bb) {
		return (bb >> 6) & BitBoard::not_ab_file;
	}

	INLINE BitBoard soSoEa(BitBoard bb) {
		return (bb >> 15) & BitBoard::not_a_file;
	}

	INLINE BitBoard soSoWe(BitBoard bb) {
		return (bb >> 17) & BitBoard::not_h_file;
	}

	INLINE BitBoard soWeWe(BitBoard bb) {
		return (bb >> 10) & BitBoard::not_gh_file;
	}

	INLINE BitBoard noWeWe(BitBoard bb) {
		return (bb << 6) & BitBoard::not_gh_file;
	}

	INLINE BitBoard noNoWe(BitBoard bb) {
		return (bb << 15) & BitBoard::not_h_file;
	}

	INLINE BitBoard inBetween(Square org, Square dst) {
		assert(org.isValid() and org.isNotNull() and dst.isValid() and dst.isNotNull());
		return rectangular.table[org][dst];
	}

} // namespace