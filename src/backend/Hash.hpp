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

#include "Common.hpp"

class Position;

class ZobristMasks {
public:
    ZobristMasks(const ZobristMasks&) = delete;
    ZobristMasks(ZobristMasks&&) = delete;

    ZobristMasks& operator=(const ZobristMasks&) = delete;
    ZobristMasks& operator=(ZobristMasks&&) = delete;

    _NODISCARD static ZobristMasks& get();

    uint64_t                    black_key;
    MultiArray<uint64_t, 8>        ep_file_keys;
    MultiArray<uint64_t, 2>        short_castle_keys;
    MultiArray<uint64_t, 2>        long_castle_keys;
    MultiArray<uint64_t, 2, 6, 64> piece_keys;
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

#if defined(DEBUG)
    bool printXOR_Diff(uint64_t key_2);
#endif

    static constexpr uint64_t Undef = 0;
private:
    uint64_t _key = Undef;
};
