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

#include <array>
#include <cstddef>
#include <iterator>

namespace multiarr {

template<typename T, size_t N, size_t... Ns>
struct MultiArray {
    using type = std::array<typename MultiArray<T, Ns...>::type, N>;
};

template<typename T, size_t N>
struct MultiArray<T, N> {
    using type = std::array<T, N>;
};

template<typename T, size_t... Ns>
struct MultiArrayWrapper {
    using MultiArray = typename MultiArray<T, Ns...>::type;

    using value_type             = T;
    using size_type              = size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type Count = (Ns * ...);

    MultiArray arr;

    constexpr auto& operator[](size_type pos) noexcept { return arr[pos]; }
    constexpr const auto& operator[](size_type pos) const noexcept { return arr[pos]; }

    constexpr bool operator==(const MultiArrayWrapper& a) const noexcept { return arr == a.arr; }

    constexpr auto& at(size_type pos) { return arr.at(pos); }
    constexpr const auto& at(size_type pos) const { return arr.at(pos); }

    constexpr auto& front() noexcept { return arr.front(); }
    constexpr const auto& front() const noexcept { return arr.front(); }

    constexpr auto& back() noexcept { return arr.back(); }
    constexpr const auto& back() const noexcept { return arr.back(); }

    constexpr pointer data() noexcept { 
        return reinterpret_cast<pointer>(&arr); 
    }
    constexpr const_pointer data() const noexcept { 
        return reinterpret_cast<const_pointer>(&arr); 
    }

    constexpr iterator begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr const_iterator cbegin() const noexcept { return data(); }

    constexpr iterator end() noexcept { return data() + Count; }
    constexpr const_iterator end() const noexcept { return data() + Count; }
    constexpr const_iterator cend() const noexcept { return data() + Count; }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
};

} // namespace multiarr

template<typename T, size_t... Ns>
using MultiArray = multiarr::MultiArrayWrapper<T, Ns...>;
