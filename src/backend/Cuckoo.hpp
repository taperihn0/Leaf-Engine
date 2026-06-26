/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
    static_assert(isExp2(_CuckooTableSize));

    static constexpr uint    _KickThreshold = 216;
    static constexpr size_t _AccurateCount = 2212;

    struct CuckooEntry {
        uint32_t move_hash;
        Move16b  move16;
    };

    std::unique_ptr<CuckooEntry[]> _cuckoo_entry_buff;
};


