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
		return (hash >> 16) & (_CuckooTableSize - 1);
	}

	_FORCEINLINE uint32_t getMoveHash(size_t idx) const {
		assert(idx < _CuckooTableSize);
		return _cuckoo_entry_buff[idx].move_hash;
	}

	_FORCEINLINE Move16b getMove16b(size_t idx) const {
		assert(idx < _CuckooTableSize);
		return _cuckoo_entry_buff[idx].move16;
	}

	static _FORCEINLINE size_t getSize() {
		return _CuckooTableSize;
	}

private:
	void validate();

	static constexpr size_t _CuckooTableSize = 4096;
	static_assert(isPow2(_CuckooTableSize));

	static constexpr uint	_KickThreshold = 216;
	static constexpr size_t _AccurateCount = 2212;

	struct _CuckooEntry {
		uint32_t move_hash;
		Move16b  move16;
	};

	_CuckooEntry* _cuckoo_entry_buff;
};


