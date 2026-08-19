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

namespace mvorder::mvhist {    

static constexpr int16_t MaxAbsContinuationValue = 8192;
static constexpr int16_t MaxAbsHistoryValue = 8192;

template <typename T, T MaxAbsValue, typename = std::enable_if_t<std::is_integral_v<T>>>
class HistoryEntry {
public:
    using value_type = T;

    HistoryEntry() = default;

    HistoryEntry& operator=(T v);

    _INLINE void reduce() noexcept;
    _NODISCARD _FORCEINLINE value_type value() const noexcept;

    template <int8_t Sign, typename = std::enable_if_t<Sign == -1 or Sign == 1>>
    _FORCEINLINE static void applyGravityFormula(HistoryEntry& entry, T bonus);

    static constexpr T MaxAbsBound = MaxAbsValue;
private:
    T _v = 0;
};

class ContinuationSubtable {
public:
    using Entry = HistoryEntry<int16_t, MaxAbsContinuationValue>;

    ContinuationSubtable() = default;
    ContinuationSubtable(const ContinuationSubtable&) = delete;
    ContinuationSubtable(ContinuationSubtable&&) = delete;

    ContinuationSubtable& operator=(const ContinuationSubtable&) = delete;
    ContinuationSubtable& operator=(ContinuationSubtable&&) = delete;

    void clear();

    template <int8_t Sign, typename = std::enable_if_t<Sign == -1 or Sign == 1>>
    _FORCEINLINE void update(enumColor side, Move32b move, int16_t bonus);

    _NODISCARD _FORCEINLINE Entry::value_type getValue(enumColor side, Move32b move);

    Entry* begin();
    Entry* end();
private:
    MultiArray<Entry, 2, 6, 64> _continuation_subtable;
};

class ContinuationTable {
public:
    ContinuationTable() = default;
    ContinuationTable(const ContinuationTable&) = delete;
    ContinuationTable(ContinuationTable&&) = delete;

    ContinuationTable& operator=(const ContinuationTable&) = delete;
    ContinuationTable& operator=(ContinuationTable&&) = delete;

    void clear();
    _NODISCARD _FORCEINLINE ContinuationSubtable& getSubtable(enumColor side, Move32b move);
    _INLINE void reduce(); 
private:
    MultiArray<ContinuationSubtable, 2, 6, 64> _continuation_tables;
};

class HistoryTable {
public:
    using Entry = HistoryEntry<int16_t, MaxAbsHistoryValue>;

    HistoryTable() = default;

    void clear();
    template <int8_t Sign, typename = std::enable_if_t<Sign == -1 or Sign == 1>>
    _FORCEINLINE void update(enumColor side, Move32b move, int16_t bonus);
    _INLINE void reduce(); 
    _NODISCARD _FORCEINLINE Entry::value_type getValue(enumColor side, Move32b move);
private:
    MultiArray<Entry, 2, 6, 64> _quiets_history;
};

template <typename T, T MaxAbsValue, typename _/* = std::enable_if_t<std::is_integral_v<T>> */>
HistoryEntry<T, MaxAbsValue, _>& HistoryEntry<T, MaxAbsValue, _>::operator=(T v) { 
    _v = v; 
    return *this; 
}

template <typename T, T MaxAbsValue, typename _/* = std::enable_if_t<std::is_integral_v<T>> */>
_INLINE void HistoryEntry<T, MaxAbsValue, _>::reduce() noexcept {
    _v /= 2;
}

template <typename T, T MaxAbsValue, typename _/* = std::enable_if_t<std::is_integral_v<T>> */>
_NODISCARD _FORCEINLINE T HistoryEntry<T, MaxAbsValue, _>::value() const noexcept {
    return _v;
}

template <typename T, T MaxAbsValue, typename _/* = std::enable_if_t<std::is_integral_v<T>> */>
template <int8_t Sign, typename __ /* = std::enable_if_t<Sign == -1 or Sign == 1> */>
_FORCEINLINE void HistoryEntry<T, MaxAbsValue, _>::applyGravityFormula(HistoryEntry& entry, T bonus) {
    const int32_t mbonus = std::min(bonus, MaxAbsBound);

    entry._v += static_cast<T>(
        Sign * mbonus - entry.value() * mbonus / MaxAbsBound
    );
}

_INTERNAL void ContinuationSubtable::clear() {
    std::fill(std::begin(_continuation_subtable), std::end(_continuation_subtable), 0);
}

_FORCEINLINE std::pair<Piece::uint_t, Square> extractMoveIndexes(Move32b move) {
    const Piece::uint_t piece = index(move.getPiece());
    const Square to = move.getTarget();
    return std::make_pair(piece, to);
}

template <int8_t Sign, typename _ /* = std::enable_if_t<Sign == -1 or Sign == 1> */>
_FORCEINLINE void ContinuationSubtable::update(enumColor side, Move32b move, int16_t bonus) {
    const auto& [piece, to] = extractMoveIndexes(move);
    Entry& entry = _continuation_subtable[side][piece][to];
    Entry::applyGravityFormula<Sign>(entry, bonus);
}

_FORCEINLINE ContinuationSubtable::Entry::value_type ContinuationSubtable::getValue(enumColor side, Move32b move) {
    const auto& [piece, to] = extractMoveIndexes(move);
    Entry& entry = _continuation_subtable[side][piece][to];
    return entry.value();
}

_INTERNAL ContinuationSubtable::Entry* ContinuationSubtable::begin() { 
    return reinterpret_cast<Entry*>(_continuation_subtable.begin()); 
}

_INTERNAL ContinuationSubtable::Entry* ContinuationSubtable::end() { 
    return reinterpret_cast<Entry*>(_continuation_subtable.end()); 
}

_INTERNAL void ContinuationTable::clear() {
    for (auto& subtable : _continuation_tables)
        subtable.clear();
}

_NODISCARD _FORCEINLINE ContinuationSubtable& ContinuationTable::getSubtable(enumColor side, Move32b move) {
    const auto& [piece, to] = extractMoveIndexes(move);
    return _continuation_tables[side][piece][to];
}

_INTERNAL void ContinuationTable::reduce() {
    for (auto& subtable : _continuation_tables) 
        for (auto& entry : subtable)
            entry.reduce();
}

_INTERNAL void HistoryTable::clear() {
    std::fill(std::begin(_quiets_history), std::end(_quiets_history), 0);
}

template <int8_t Sign, typename _ /* = std::enable_if_t<Sign == -1 or Sign == 1> */>
_FORCEINLINE void HistoryTable::update(enumColor side, Move32b move, int16_t bonus) {
    const auto& [piece, to] = extractMoveIndexes(move);
    Entry& entry = _quiets_history[side][piece][to];
    Entry::applyGravityFormula<Sign>(entry, bonus);
}

_INTERNAL void HistoryTable::reduce() {
    for (auto& entry : _quiets_history) 
        entry.reduce();
}

_NODISCARD _FORCEINLINE HistoryTable::Entry::value_type HistoryTable::getValue(enumColor side, Move32b move) {
    const auto& [piece, to] = extractMoveIndexes(move);
    return _quiets_history[side][piece][to].value();
}

} // namespace mvorder::mvhist