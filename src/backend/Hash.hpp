#pragma once

#include "Common.hpp"

class Position;

class ZobristMasks {
public:
	ZobristMasks(const ZobristMasks&) = delete;
	ZobristMasks(ZobristMasks&&) = delete;

	ZobristMasks& operator=(const ZobristMasks&) = delete;
	ZobristMasks& operator=(ZobristMasks&&) = delete;

	static ZobristMasks& get();

	uint64_t 					black_key;
	array1d<uint64_t, 8> 	ep_file_keys;
	array1d<uint64_t, 2> 	short_castle_keys;
	array1d<uint64_t, 2> 	long_castle_keys;
	array3d<uint64_t, 2, 6, 64> piece_keys;
private:
	ZobristMasks();
	void fillKeys();
};

inline const ZobristMasks* ZHashMasks = &ZobristMasks::get();

class ZHash {
public:	
	constexpr ZHash() = default;
	
	_INLINE constexpr ZHash(uint64_t key) 
		: _key(key) {}

	_INLINE constexpr operator uint64_t() const {
		return _key;
	}

	_INLINE constexpr ZHash& operator=(const ZHash& zh) {
		_key = zh._key;
		return *this;
	}

	_INLINE constexpr ZHash operator^=(const ZHash& zh) {
		return _key ^= zh._key;
	}

	_INLINE constexpr ZHash operator^(const ZHash& zh) const {
		return _key ^ zh._key;
	}

	static ZHash generateOnFly(const Position& pos);

#if defined(_DEBUG)
	bool printXOR_Diff(uint64_t key_2);
#endif
private:
	uint64_t _key = 0;
};
