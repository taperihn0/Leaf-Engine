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

class Score {
public:
    using int_t = int16_t;

    _INLINE Score() = default;
    _INLINE constexpr Score(int_t val)
        : _raw(val) {
    }

    _INLINE Score operator+(Score b) const {
        return _raw + b._raw;
    }

    _INLINE Score operator+=(Score b) {
        return _raw += b._raw;
    }

    _INLINE Score operator-=(Score b) {
        return _raw -= b._raw;
    }

    _INLINE Score operator-(Score b) const {
        return _raw - b._raw;
    }

    _INLINE bool operator>(Score b) const {
        return _raw > b._raw;
    }

    _INLINE bool operator>=(Score b) const {
        return _raw >= b._raw;
    }

    _INLINE bool operator<=(Score b) const {
        return _raw <= b._raw;
    }

    _INLINE bool operator==(Score b) const {
        return _raw == b._raw;
    }

    _INLINE bool operator!=(Score b) const {
        return _raw != b._raw;
    }

    _INLINE bool operator<(Score b) const {
        return _raw < b._raw;
    }

    _INLINE Score operator*(Score b) const {
        return _raw * b._raw;
    }

    _INLINE Score operator*(long double d) const {
        return static_cast<Score::int_t>(_raw * d);
    }

    _INLINE Score operator/(Score b) const {
        return _raw / b._raw;
    }

    _INLINE Score operator-() const {
        return -_raw;
    }

    _INLINE explicit operator int_t() const {
        return _raw;
    }

    _INLINE explicit operator int() const {
        return _raw;
    }

    _INLINE explicit operator int64_t() const {
        return _raw;
    }

    _INLINE explicit operator float() const {
        return static_cast<float>(_raw);
    }

    _NODISCARD _INLINE bool isValid() const {
        return _raw != Undef and _raw != -Undef;
    }

    _NODISCARD _INLINE bool isMateScore() const {
        return isValid() and 
               ((_raw >= -Mate and _raw < -MateBound) or 
                   (_raw > MateBound and _raw <= Mate));
    }

    _NODISCARD static _INLINE Score getMateScore(int ply) {
        return static_cast<Score>(Mate - ply);
    }

    _NODISCARD _INTERNAL std::string toStr() const {
        if (_raw > MateBound)
            return "mate " + std::to_string((Score::Mate - _raw + 1) / 2);
        else if (_raw < -MateBound)
            return "mate -" + std::to_string((_raw + Score::Mate + 1) / 2);

        return "cp " + std::to_string(_raw);
    }
    
    static constexpr int_t Draw       = 0,
                           Mate       = 32000,
                           MateBound  = Mate - MaxDepth,
                           Infinity   = maxof<int_t>(),
                           Undef      = 32500;
private:
    int_t _raw;
};
