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

template <typename T>
class TunableParameterValue {
public:
    TunableParameterValue(T v,
                          const std::string& param, 
                          void* addr, 
                          float mi, 
                          float ma, 
                          float step_rate);
    explicit operator T() const { return _v; }
private:
    T _v;
};

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

    using iterator = std::vector<std::pair<std::string, ParameterInfo>>::iterator;
    using const_iterator = std::vector<std::pair<std::string, ParameterInfo>>::const_iterator;

    iterator begin();
    iterator end();
    const_iterator cbegin() const;
    const_iterator cend() const;
private:
    std::unordered_map<std::string, ParameterInfo> _params;
    std::vector<std::pair<std::string, ParameterInfo>> _order;
};

inline TunableParametersMap GlobParamMapping;

template <typename T>
TunableParameterValue<T>::TunableParameterValue(T v,
                                              const std::string& param, 
                                              void* addr, 
                                              float mi, 
                                              float ma, 
                                              float step_rate)
    : _v(v) {
    GlobParamMapping.addTunableParameter(param, addr, mi, ma, step_rate);  
}

#define _DEFINE_TUNABLE_PARAMETER(parameter, type, value , mi, ma, step_rate)                          \
_PARAM_ATTRIBS type parameter = static_cast<type>(TunableParameterValue<type>(roundi<float>(value),    \
                                                                              std::string(#parameter), \
                                                                              &parameter,              \
                                                                              mi,                      \
                                                                              ma,                      \
                                                                              step_rate))    

#else 

#define _DEFINE_TUNABLE_PARAMETER(parameter, type, value , mi, ma, step_rate) \
_PARAM_ATTRIBS type parameter = static_cast<type>(roundi<float>(value));

#endif // _ENABLE_TUNING
