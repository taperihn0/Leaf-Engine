#pragma once

#include "Common.hpp"

class Position;

class ZobristHash {
public:
	friend class Position;

	inline ZobristHash() { fillKeys(); }

	void fillKeys();
	uint64_t generateOnFly(const Position& pos);

#if defined(_DEBUG)
	bool printXOR_Diff(uint64_t key_2);
#endif

private:
	// C-style multidimensional arrays are used there simply because of 
	// simplicity in declaration in opposite to std::array class template
	uint64_t _piece_keys[2][6][64];
	uint64_t _black_key;
	uint64_t _ep_file_keys[8];
	uint64_t _short_castle_keys[2], _long_castle_keys[2];

	uint64_t _key = 0;
};