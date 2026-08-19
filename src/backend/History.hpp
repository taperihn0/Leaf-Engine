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

namespace mvorder::hist {    

class HistoryTables {
public:
    friend class MoveOrder;
    
    HistoryTables();
    void clearHistoryTables();
    void onNewSearch();

    _NODISCARD static constexpr int16_t getContinuationPly() { return _ContinuationPly; }
private:
    _NODISCARD _INLINE int16_t getNormalizedHistQuietScore(Move32b move, enumColor side);

    static inline constexpr int16_t _MaxAbsQuietsHistory = 8192;
    static inline constexpr int16_t _MaxAbsContinuationHistory = 8192;
    static inline constexpr int16_t _ContinuationPly = 2;
    
    MultiArray<int16_t, 2, 6, 64>      _quiets_history;
    MultiArray<MultiArray<MultiArray<int16_t, 2, 6, 64>, 2, 6, 64>, _ContinuationPly>
                                    _cont_history;
};

} // namespace mvorder::hist
