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
#include "Score.hpp"
#include "Move.hpp"
#include "Memory.hpp"

struct SearchResults;

static constexpr size_t  EntryTargetSize  = 10;
static constexpr size_t  BucketTargetSize = 32;
static constexpr size_t  EntryKeySize     = 18;
static constexpr uint8_t EntryMaxDepth    = 64;

enum class TTBound : uint8_t {
    NONE       = 0,
    EXACT      = 1,
    UPPERBOUND = 2,
    LOWERBOUND = 3,
    MAX_BOUND  = 3
};

struct TTEntry {
    TTEntry();

    _NODISCARD _INLINE bool isEmpty() const noexcept { 
        return depth == 0 and bound == TTBound::NONE; 
    }

    _INLINE void writeHash(uint32_t keyhi) noexcept {
        key16 = static_cast<uint16_t>(keyhi);
        key18 = (keyhi & 0x30000) >> 16;
    }

    _NODISCARD _INLINE uint32_t getHash() const noexcept {
        return (static_cast<uint32_t>(key18) << 16) | key16;
    }

    uint16_t key16;
    uint8_t  key18 : 2;
    uint8_t  generation : 6;
    TTBound  bound : 2;
    uint8_t  depth : 6;
    Score    score;
    Move16b  move;
    Score    eval;
};

static_assert(sizeof(TTEntry) == EntryTargetSize);

struct alignas(BucketTargetSize) Bucket {
    static constexpr size_t InternalEntriesCnt = 3;
    static constexpr size_t AlignmentSize = BucketTargetSize - InternalEntriesCnt * EntryTargetSize;

    array1d<TTEntry, InternalEntriesCnt> entries;
    array1d<std::byte, AlignmentSize> __align;
};

static_assert(sizeof(Bucket) == BucketTargetSize);
static_assert(sizeof(Bucket) == alignof(Bucket));

static constexpr size_t DefaultTTSizeMb = 256_MB;

class TranspositionTable {
public:
    explicit TranspositionTable(size_t mb_size = DefaultTTSizeMb);
    TranspositionTable(TranspositionTable&& tt) = default;
    TranspositionTable& operator=(TranspositionTable&&) = default;

    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    void resize(size_t size_mb);
    void clear();

    void write(uint64_t node_key, 
               uint8_t node_depth, 
               uint8_t node_ply, 
               TTBound node_bound, 
               Score node_score, 
               Move16b node_move, 
               Score node_eval);

    bool probe(TTEntry& out_entry,
               uint64_t key, 
               Score alpha, Score beta, 
               uint8_t node_depth) const;

    void prefetchBucket(uint64_t key64) const;

#if defined(DEBUG)
    void printDebug();
#endif

    _NODISCARD size_t getEntriesCount() const;
    _NODISCARD uint16_t getHashfull() const;

    void newGeneration();
    void clearHashfull();
private:
    mem::PageAlignedUniquePtr<Bucket> getPageAlignedMemoryHandle(size_t bucket_cnt);

    mem::PageAlignedUniquePtr<Bucket> 
            _mem;
    size_t  _buckets_cnt;
    uint8_t _buckets_pow_2;
    uint8_t _generation;
    ull     _hits;
};
