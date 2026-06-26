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

enum enumColor : bool {
    WHITE = 0, 
    BLACK
};

_NODISCARD _INLINE constexpr enumColor operator!(enumColor opp) {
    return static_cast<enumColor>(!static_cast<bool>(opp));
}

class Turn {
public:
    _INLINE Turn() = default;
    
    _INLINE constexpr Turn(enumColor c)
        : _col(c) {}

    _NODISCARD _INLINE constexpr operator enumColor() const {
        return _col;
    }

    _INLINE constexpr Turn operator=(enumColor c) {
        return _col = c;
    }

    _NODISCARD _INLINE constexpr Turn operator!() const {
        return !_col;
    }

    _INLINE void fromChar(char c) {
        _col = c == 'w' ? WHITE : BLACK;
    }

    void print(std::ostream& os = std::cout) const {
        os << (_col == WHITE ? 'w' : 'b');
    }
private:
    enumColor _col;
};
