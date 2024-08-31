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

	constexpr BitBoard(uint64_t raw_init)
		: _board(raw_init) {}

	constexpr BitBoard(Square sq)
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

	// debug-purpose method
	void printRaw();
	
	void set(uint64_t bb);

	int popCount() const;
	int bitScanForward() const;
	int bitScanReverse() const;

	// bit scan forward but with LS1B reset
	int dropForward();

	INLINE void popBit(int shift) {
		assert(shift < 64);
		_board &= ~(1Ui64 << shift);
	}

	INLINE void setBit(int shift) {
		assert(shift < 64);
		_board |= (1Ui64 << shift);
	}

	INLINE bool getBit(int shift) const {
		assert(shift < 64);
		return _board & (1Ui64 << shift);
	}

	INLINE void moveBit(int origin, int target) {
		popBit(origin);
		setBit(target);
	}

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

} // namespace