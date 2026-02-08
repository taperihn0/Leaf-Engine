#pragma once

#include "Move.hpp"

class CuckooTables {
public:
	CuckooTables();
	void init();

	static _FORCEINLINE size_t cuckooIndex1(uint64_t hash) {
		return hash & (_CuckooTableSize - 1);
	}

	static _FORCEINLINE size_t cuckooIndex2(uint64_t hash) {
		return (hash >> 14) & (_CuckooTableSize - 1);
	}

	_FORCEINLINE uint64_t getMoveHash(size_t idx) const {
		assert(idx < _CuckooTableSize);
		return _cuckoo_move_hash_buff[idx];
	}

	_FORCEINLINE Move16b getMove16b(size_t idx) const {
		assert(idx < _CuckooTableSize);
		return _cuckoo_move16_buff[idx];
	}

	static _FORCEINLINE size_t getSize() {
		return _CuckooTableSize;
	}

private:
	void validate();

	static constexpr size_t _CuckooTableSize = 8192;
	static_assert(isPow2(_CuckooTableSize));

	static constexpr uint	_KickThreshold = 216;
	static constexpr size_t _AccurateCount = 2212;

	uint64_t* _cuckoo_move_hash_buff;
	Move16b*  _cuckoo_move16_buff;
};


