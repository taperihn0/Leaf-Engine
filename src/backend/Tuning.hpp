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

#include <unordered_map>

#if defined(_ENABLE_TUNING)

class TunableParametersMap {
public:
    TunableParametersMap() = default;
    void addTunableParameter(const std::string& param, 
                             void* addr, 
                             float mi, 
                             float ma, 
                             float step_rate);
    _NODISCARD void* getAddressOf(const std::string& str);

    struct ParameterInfo {
        void* addr;
        float mi, ma;
        float step_rate;
    };

    std::unordered_map<std::string, ParameterInfo>::iterator begin();
    std::unordered_map<std::string, ParameterInfo>::iterator end();
    std::unordered_map<std::string, ParameterInfo>::const_iterator cbegin() const;
    std::unordered_map<std::string, ParameterInfo>::const_iterator cend() const;
private:
    std::unordered_map<std::string, ParameterInfo> _params;
};

inline TunableParametersMap GlobParamMapping;

#define _DEFINE_TUNABLE_PARAMETER(parameter, type, value , mi, ma, step_rate)                     \
_PARAM_ATTRIBS type parameter = [](std::string_view param_name) {                                 \
    GlobParamMapping.addTunableParameter(std::string(param_name), &parameter, mi, ma, step_rate); \
    return static_cast<type>(roundi<float>(value));                                               \
}(#parameter)

#else 

#define _DEFINE_TUNABLE_PARAMETER(parameter, type, value , mi, ma, step_rate) \
_PARAM_ATTRIBS type parameter = static_cast<type>(roundi<float>(value));

#endif // _ENABLE_TUNING
