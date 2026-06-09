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

#include "Common.hpp"

#include <unordered_map>

#if defined(_ENABLE_TUNING)

class TunableParametersMap {
public:
    TunableParametersMap() = default;
    void createMapping();
    _NODISCARD void* getAddressOf(const std::string& str);
private:
    std::unordered_map<std::string, void*> _addr;
};

extern TunableParametersMap GlobParamMapping;

#endif // _ENABLE_TUNING
