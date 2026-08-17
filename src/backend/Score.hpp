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

namespace sc {

namespace util {

template <typename Derived>
class ScoreBase {
public:
    using int_t = int16_t;

    ScoreBase() noexcept = default;
    _INLINE constexpr ScoreBase(int32_t val) noexcept
        : _v(static_cast<int_t>(val)) {}

    _FORCEINLINE constexpr Derived  operator-()           const noexcept { return Derived(-_v); }
    _FORCEINLINE constexpr Derived  operator+()           const noexcept { return Derived(_v); }
    _FORCEINLINE constexpr Derived  operator+(Derived b)  const noexcept { return Derived(_v + b._v); }
    _FORCEINLINE constexpr Derived  operator-(Derived b)  const noexcept { return Derived(_v - b._v); }
    _FORCEINLINE constexpr Derived  operator*(Derived b)  const noexcept { return Derived(_v * b._v); }
    _FORCEINLINE constexpr Derived  operator/(Derived b)  const noexcept { return Derived(_v / b._v); }

    _FORCEINLINE constexpr bool     operator>(Derived b)  const noexcept { return _v > b._v; }
    _FORCEINLINE constexpr bool     operator>=(Derived b) const noexcept { return _v >= b._v; }
    _FORCEINLINE constexpr bool     operator<=(Derived b) const noexcept { return _v <= b._v; }
    _FORCEINLINE constexpr bool     operator<(Derived b)  const noexcept { return _v < b._v; }
    _FORCEINLINE constexpr bool     operator==(Derived b) const noexcept { return _v == b._v; }
    _FORCEINLINE constexpr bool     operator!=(Derived b) const noexcept { return _v != b._v; }

    _FORCEINLINE constexpr explicit operator int_t()      const noexcept { return _v; }
    _FORCEINLINE constexpr explicit operator int32_t()    const noexcept { return _v; }
    _FORCEINLINE constexpr explicit operator int64_t()    const noexcept { return _v; }
    _FORCEINLINE constexpr explicit operator float()      const noexcept { return static_cast<float>(_v); }

    _FORCEINLINE constexpr Derived& operator=(int32_t v) noexcept {
        _v = static_cast<int_t>(v);
        return asDerived();
    }

    _FORCEINLINE constexpr Derived& operator=(const Derived& s) noexcept {
        _v = s._v;
        return asDerived();
    }

    _FORCEINLINE constexpr Derived& operator+=(Derived b) noexcept {
        _v += b._v;
        return asDerived();
    }

    _FORCEINLINE constexpr Derived& operator-=(Derived b) noexcept {
        _v -= b._v;
        return asDerived();
    }

    _NODISCARD _INLINE constexpr int16_t value() const noexcept { return _v; }

    _NODISCARD _INLINE constexpr bool isValid() const;

    int_t _v;
private:
    constexpr Derived& asDerived() noexcept {
        return static_cast<Derived&>(*this);
    }

    constexpr const Derived& asDerived() const noexcept {
        return static_cast<const Derived&>(*this);
    }
};

} // namespace util

/* That is int16_t wrapper that express
*  board evaluation in centipawns.
*/
class Score final : public util::ScoreBase<Score> {
public:
    friend class util::ScoreBase<Score>;

    Score() = default;
    _INLINE constexpr Score(const Score& s) = default;
    _INLINE constexpr Score(int32_t val) noexcept
        : ScoreBase(val) {}

    _NODISCARD _INLINE std::string toStr() const; 
    _NODISCARD _INLINE constexpr bool isMateScore() const;
    _NODISCARD static _INLINE constexpr Score getMateScore(int ply);
private:
    using util::ScoreBase<Score>::_v;
};

inline constexpr Score Draw      = Score(0);
inline constexpr Score Win       = Score(8000);
inline constexpr Score KnownWin  = Score(12000);
inline constexpr Score Mate      = Score(32000);
inline constexpr Score MateBound = Score(32000 - MaxDepth);
inline constexpr Score Infinity  = Score(maxof<Score::int_t>());
inline constexpr Score Undef     = Score(32500);

template <typename Derived>
_NODISCARD _INLINE constexpr bool util::ScoreBase<Derived>::isValid() const {
    return _v != Undef.value() and _v != -Undef.value();
}

_NODISCARD _INLINE constexpr bool Score::isMateScore() const {
    return isValid() and (
        (_v >= -Mate.value() and _v < -MateBound.value()) or 
        (_v > MateBound.value() and _v <= Mate.value())
    );
}

_NODISCARD _INLINE constexpr Score Score::getMateScore(int ply) {
    return Score(Mate.value() - ply);
}

_NODISCARD _INLINE std::string Score::toStr() const {
    if (_v > MateBound.value())
        return "mate " + std::to_string((Mate.value() - _v + 1) / 2);

    else if (_v < -MateBound.value())
        return "mate -" + std::to_string((_v + Mate.value() + 1) / 2);

    return "cp " + std::to_string(_v);
}

} // namespace sc
