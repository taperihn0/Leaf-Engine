#pragma once

#include "Common.hpp"

class Position;

class ZobristHash {
public:
	friend class Position;

	static void fillKeys();
	uint64_t generateOnFly(const Position& pos);

#if defined(_DEBUG)
	bool printXOR_Diff(uint64_t key_2);
#endif

	void set(uint64_t key) { _key = key; }
private:
	static inline uint64_t _piece_keys[2][6][64];
	static inline uint64_t _black_key;
	static inline uint64_t _ep_file_keys[8];
	static inline uint64_t _short_castle_keys[2], 
			 			   _long_castle_keys[2];

	uint64_t _key = 0;
};
