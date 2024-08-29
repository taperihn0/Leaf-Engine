#include "Common.hpp"

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
	constexpr BitBoard(BitBoard&) = default;
	constexpr BitBoard(BitBoard&&) = default;

	constexpr BitBoard(uint64_t raw_init)
		: _board(raw_init) {}

	~BitBoard() = default;

	INLINE constexpr BitBoard operator=(const BitBoard& cpy) {
		return _board = cpy._board;
	}

	INLINE constexpr BitBoard operator|(BitBoard bb) const {
		return _board | bb._board;
	}

	INLINE constexpr BitBoard operator^(BitBoard bb) const {
		return _board ^ bb._board;
	}

	INLINE constexpr BitBoard operator^(uint64_t bb) const {
		return _board ^ bb;
	}

	// debug-purpose method
	void PrintRaw();
	
	void SetRaw(uint64_t bb);

	int popCount();
	int bitScanForward();
	int bitScanReverse();

	INLINE void popBit(int shift) {
		assert(shift < 64);
		_board &= ~(1Ui64 << shift);
	}

	INLINE void setBit(int shift) {
		assert(shift < 64);
		_board |= (1Ui64 << shift);
	}

	INLINE bool getBit(int shift) {
		assert(shift < 64);
		return _board & (1Ui64 << shift);
	}

	INLINE void moveBit(int origin, int target) {
		popBit(origin);
		setBit(target);
	}

	static constexpr uint64_t universe = 0xffffffffffffffffUi64;
	static constexpr uint64_t empty = 0Ui64;
private:
	uint64_t _board;
};