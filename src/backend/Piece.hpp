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
#include "Color.hpp"
#include "StaticEval.hpp"

class Piece {
public:
    using uint_t = uint8_t;

    enum enumType : uint_t {
        PAWN,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
        NONE,
    };

    Piece() = default;
    explicit Piece(enumType piece_type) { set(WHITE, piece_type); }
    explicit Piece(enumColor col_t, enumType piece_type) { set(col_t, piece_type); }

    _NODISCARD static Piece fromChar(enumColor col_t, char c) {
        auto id = col_t == WHITE ? _WhitesStr.find_first_of(c)
                                 : _BlacksStr.find_first_of(c);
        return Piece(col_t, enumType(id));
    }

    _NODISCARD static enumType typeFromChar(char c) {
        c = tolower(c);
        auto id = _BlacksStr.find_first_of(c);
        return enumType(id);
    }

    void set(enumColor col_t, enumType piece_type) { 
        _col = col_t;
        _type = piece_type;
    }

    void setColor(enumColor col) {
        _col = col;
    }

    void setType(enumType piece_type) {
        _type = piece_type;
    }

    void print(std::ostream& os = std::cout) const {
        if (_type == NONE) os << ' ';
        else if (_col == WHITE) os << _WhitesStr[_type];
        else os << _BlacksStr[_type];
    }

    _NODISCARD _INLINE Piece::uint_t value() const {
        return static_cast<Piece::uint_t>(_type);
    }

    _NODISCARD _INLINE enumType type() const {
        return _type;
    }

    _NODISCARD _INLINE enumColor color() const {
        return _col;
    }

    static constexpr MultiArray<enumType, 5> PieceTypeWithoutKingList = {
        Piece::PAWN, 
        Piece::KNIGHT, 
        Piece::BISHOP, 
        Piece::ROOK, 
        Piece::QUEEN, 
    };

    static constexpr MultiArray<enumType, 6> PieceTypeList = { 
        Piece::PAWN, 
        Piece::KNIGHT, 
        Piece::BISHOP, 
        Piece::ROOK, 
        Piece::QUEEN,
        Piece::KING,
    };

private:
    static constexpr std::string_view _WhitesStr = "PNBRQK", 
                                      _BlacksStr = "pnbrqk";
    enumType _type;
    enumColor _col;
};

_INLINE std::ostream& operator<<(std::ostream& os, Piece p) {
    p.print(os);
    return os;
}

_NODISCARD _FORCEINLINE constexpr Piece::uint_t index(Piece::enumType p) {
    return static_cast<Piece::uint_t>(p);
}

_NODISCARD _FORCEINLINE constexpr bool isSlider(Piece::enumType p) {
    return p >= Piece::BISHOP and p <= Piece::QUEEN;
}

_NODISCARD _FORCEINLINE int pieceValue(Piece::enumType p) {
    return *PieceValue[index(p)];
}
