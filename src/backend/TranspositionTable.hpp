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

namespace tt {

static constexpr size_t  EntryTargetSize  = 10;
static constexpr size_t  BucketTargetSize = 32;
static constexpr size_t  EntryKeySize     = 18;
static constexpr uint8_t EntryMaxDepth    = 63;   /* depth is a 6-bit field, 64 would wrap to 0 */
static constexpr uint8_t GenerationMask   = 0x3F; /* 6-bit generation wheel */

enum class Bound : uint8_t {
    NONE  = 0,
    EXACT = 1,
    UPPER = 2,
    LOWER = 3,
    MAX   = 3
};

#pragma pack(push, 1)

struct TTEntryPlacementSpecific {
    using key_value_type = uint32_t;

    TTEntryPlacementSpecific() = default;

    _FORCEINLINE void writeHashKey(key_value_type keyhi) noexcept {
        low_key = lowKeyPart(keyhi);
        high_key = highKeyPart(keyhi) >> 16;
    }

    _NODISCARD static _FORCEINLINE key_value_type lowKeyPart(key_value_type key) noexcept {
        return key & 0x0FFFF;
    }

    _NODISCARD static _FORCEINLINE key_value_type highKeyPart(key_value_type key) noexcept {
        return key & 0x30000;
    }

    _NODISCARD _FORCEINLINE key_value_type getHash() const noexcept {
        return (static_cast<key_value_type>(high_key) << 16) | low_key;
    }

    uint16_t low_key;
    uint8_t  high_key : 2;
    uint8_t  generation : 6;
};

struct TTEntryValues {
    TTEntryValues() = default;
    
    _NODISCARD _FORCEINLINE static TTEntryValues getClearEntryValues() noexcept {
        return TTEntryValues { 
            Bound::NONE, 
            0, 
            sc::Undef, 
            NullMove, 
            sc::Undef 
        };
    }

    _NODISCARD _FORCEINLINE bool isEmpty() const noexcept { 
        return bound == Bound::NONE; 
    }

    Bound     bound : 2;
    uint8_t   depth : 6;
    sc::Score score;
    Move16b   move;
    sc::Score eval;
};

struct TTEntry final : public TTEntryPlacementSpecific, TTEntryValues {
    TTEntry() = default;
};

#pragma pack(pop)

static_assert(sizeof(TTEntry) == EntryTargetSize);

struct alignas(BucketTargetSize) Bucket {
    static constexpr size_t InternalEntriesCnt = 3;
    static constexpr size_t AlignmentSize = BucketTargetSize - InternalEntriesCnt * EntryTargetSize;

    std::array<TTEntry, InternalEntriesCnt> entries;
    std::array<std::byte, AlignmentSize> __align;
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
               _MAYBE_UNUSED uint8_t node_ply, 
               Bound node_bound, 
               sc::Score node_score, 
               Move16b node_move, 
               sc::Score node_eval);

    bool probe(TTEntryValues& out_entry,
               uint64_t key, 
               sc::Score alpha, sc::Score beta, 
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
    _NODISCARD static mem::PageAlignedUniquePtr<Bucket> getPageAlignedMemoryHandle(size_t bucket_cnt);

    _NODISCARD Bucket& getBucket(uint64_t key64);
    _NODISCARD const Bucket& getBucket(uint64_t key64) const;

    _NODISCARD uint64_t getKey(uint64_t key64) const;

    mem::PageAlignedUniquePtr<Bucket> 
            _mem;
    size_t  _buckets_cnt;
    uint8_t _buckets_pow_2;
    uint8_t _generation;
    ull     _hits;
};

} // namespace tt
