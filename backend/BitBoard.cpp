#include "BitBoard.hpp"

void BitBoard::printRaw() const {
	for (int h = 7; h >= 0; h--) {
		for (int i = h * 8; i < (h + 1) * 8; i++)
			std::cout << static_cast<bool>((1Ui64 << i) & _board);
		std::cout << '\n';
	}
}

void BitBoard::set(uint64_t bb) {
	_board = bb;
}

int BitBoard::popCount() const {
#if defined(__INTEL_COMPILER) or defined(_MSC_VER)
	return static_cast<int>(_mm_popcnt_u64(_board));
#elif defined(__GNUC__)
	return __builtin_popcount(_board);
#else
	uint64_t bb = _board;
	int c;
	for (c = 0; bb; bb &= bb - 1, c++);
	return c;
#endif
}

#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
int BitBoard::bitScanForward() const {
	assert(_board != 0Ui64);
	unsigned long s;
	_BitScanForward64(&s, _board);
	return static_cast<int>(s);
}

int BitBoard::bitScanReverse() const {
	assert(_board != 0Ui64);
	unsigned long s;
	_BitScanReverse64(&s, _board);
	return static_cast<int>(s);
}

#elif defined(__GNUC__)
int BitBoard::bitScanForward() const {
	assert(_board != 0Ui64);
	return __builtin_ctzll(_board);
}

int BitBoard::bitScanReverse() const {
	assert(_board != 0Ui64);
	return __builtin_clzll(_board);
}
#else

// credits to:
//  https://www.chessprogramming.org/BitScan

static constexpr int index64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

int BitBoard::bitScanForward() const {
	static constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89Ui64;
	assert(_board != 0);
	return index64[((_board ^ (_board - 1)) * debruijn64) >> 58];
}

int BitBoard::bitScanReverse() const {
	static constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89Ui64;
	uint64_t bb = _board;
	assert(_board != 0Ui64);
	bb |= bb >> 1;
	bb |= bb >> 2;
	bb |= bb >> 4;
	bb |= bb >> 8;
	bb |= bb >> 16;
	bb |= bb >> 32;
	return index64[(bb * debruijn64) >> 58];
}
#endif