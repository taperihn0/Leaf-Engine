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

#include "Tuning.hpp"
#include "MoveOrder.hpp"
#include "Search.hpp"

#if defined(_ENABLE_TUNING)

void TunableParametersMap::addTunableParameter(const std::string& param, 
                                               void* addr, 
                                               float mi, 
                                               float ma, 
                                               float step_rate) 
{
    if (_params.find(param) != _params.end())
        throw std::runtime_error("Tunable parameter already added");

    const auto info = ParameterInfo{ addr, mi, ma, step_rate };

    _params[param] = info;
    _order.push_back(std::make_pair(param, info));
}

void* TunableParametersMap::getAddressOf(const std::string& str) {
    void* base;

    try {
        base = _params.at(str).addr;
    } catch (const std::out_of_range&) {
        FAILED("Invalid parameter option: " + str);
        return nullptr;
    }

    return base;
}

TunableParametersMap::iterator TunableParametersMap::begin() {
    return _order.begin();
}

TunableParametersMap::iterator TunableParametersMap::end() {
    return _order.end();
}

TunableParametersMap::const_iterator TunableParametersMap::cbegin() const {
    return _order.cbegin();
}

TunableParametersMap::const_iterator TunableParametersMap::cend() const {
    return _order.cend();
}

#endif // _ENABLE_TUNING
