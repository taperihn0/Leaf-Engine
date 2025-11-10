#pragma once

#include "Common.hpp"

class Position;

class ZobristHash {
public:
	constexpr ZobristHash() = default;
	
	constexpr ZobristHash(uint64_t key) 
	: _key(key) {}

	INLINE constexpr operator uint64_t() const {
		return _key;
	}

	INLINE constexpr ZobristHash& operator=(const ZobristHash& zh) {
		_key = zh._key;
		return *this;
	}

	INLINE constexpr ZobristHash operator^(const ZobristHash& zh) const {
		return _key ^ zh._key;
	}

	INLINE constexpr ZobristHash operator^=(const ZobristHash& zh) {
		return _key ^= zh._key;
	}

	static void fillKeys();
	static ZobristHash generateOnFly(const Position& pos);

#if defined(_DEBUG)
	bool printXOR_Diff(uint64_t key_2);
#endif

	static inline uint64_t piece_keys[2][6][64];
	static inline uint64_t black_key;
	static inline uint64_t ep_file_keys[8];
	static inline uint64_t short_castle_keys[2], 
			 			   long_castle_keys[2];

private:
	uint64_t _key = 0;
};
