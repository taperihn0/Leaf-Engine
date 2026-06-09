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

#include "backend/Common.hpp"

// ----- States -----

enum class OptionValueType {
    TYPE_SPIN = 0,
    TYPE_BUTTON = 1
};

struct OptionType {
    virtual void print() const = 0;
};

template <typename T>
struct SpinType : public OptionType {
    using ValType = T;

    static_assert(std::is_arithmetic_v<ValType>);

    SpinType(ValType def, ValType ma, ValType mi);
    void setCurrentValue(ValType val);
    void print() const override;

    ValType default_value;
    ValType curr_value;
    ValType max_value;
    ValType min_value;
};

struct ButtonType : public OptionType {
    void print() const override;
    bool curr_value;
};

struct StringType : public OptionType {
    StringType(std::string def);
    void print() const override;
    void setCurrentValue(std::string);

    std::string default_str;
    std::string curr_str;
};

// ----- Option class -----

struct Option {
    virtual void print() const = 0;
};

// ----- Custom options -----

struct OptionHash : public Option {
    OptionHash(SpinType<ll> val);
    void print() const override;
    void set(ll val);
    ll getCurrentValue() const;

    SpinType<ll> value;
};

struct OptionClearHash : public Option {
    OptionClearHash() = default;
    void print() const override;

    ButtonType value;
};

struct OptionTunableParam : public Option {
    OptionTunableParam(SpinType<double> val, 
                       std::string option_str, 
                       double step_rate = 1.);
    void print() const override;
    void set(double val);
    double getCurrentValue() const;

    std::string      str;
    SpinType<double> value;
    double           rate;
};

struct OptionPath : public Option {
    OptionPath(const std::string& str, StringType val);
    void print() const override;
    void set(std::string fp);
    std::string getCurrentValue() const;

    StringType  value;
    std::string name;
};

// ----- Internal implementation -----

template <typename T>
_INTERNAL SpinType<T>::SpinType(ValType def, ValType mi, ValType ma)
    : default_value(def)
    , curr_value(0)
    , max_value(ma)
    , min_value(mi)
{
    setCurrentValue(def);
}

template <typename T>
_INTERNAL void SpinType<T>::setCurrentValue(ValType val) {
    curr_value = std::clamp(val, min_value, max_value);
}

template <typename T>
_INTERNAL void SpinType<T>::print() const {
    std::cout << " type spin default " << default_value 
              << " min " << min_value
              << " max " << max_value << std::endl;
}

_INTERNAL void ButtonType::print() const {
    std::cout << " type button" << std::endl;
}

_INTERNAL StringType::StringType(std::string def)
    : default_str(def)
    , curr_str("<empty>")
{}

_INTERNAL void StringType::print() const {
    std::cout << " type string" 
              << " default " << default_str << std::endl;
}

_INTERNAL void StringType::setCurrentValue(std::string str) {
    curr_str = str;
}

// ----- Custom options - implementation -----

#define _OPTION_LITERAL(name) "option name "   name
#define _OPTION_STR(name)     "option name " + name

_INTERNAL OptionHash::OptionHash(SpinType<ll> val)
    : value(val) 
{}

_INTERNAL void OptionHash::set(ll val) {
    value.setCurrentValue(val);
}

_INTERNAL ll OptionHash::getCurrentValue() const {
    return value.curr_value;
}

_INTERNAL void OptionHash::print() const {
    std::cout << _OPTION_LITERAL("Hash");
    value.print();
}

_INTERNAL void OptionClearHash::print() const {
    std::cout << _OPTION_LITERAL("Clear Hash");
    value.print();
}

_INTERNAL OptionTunableParam::OptionTunableParam(SpinType<double> val, 
                                                 std::string option_str, 
                                                 double step_rate) 
    : str(option_str)
    , value(val) 
    , rate(step_rate)
{}

_INTERNAL void OptionTunableParam::print() const {
    std::cout << _OPTION_STR(str);
    value.print();
}

_INTERNAL void OptionTunableParam::set(double val) {
    value.setCurrentValue(val);
}

_INTERNAL double OptionTunableParam::getCurrentValue() const {
    return value.curr_value;
}

_INTERNAL OptionPath::OptionPath(const std::string& str, StringType val)
    : value(val)
    , name(str)
{}

_INTERNAL void OptionPath::print() const {
    std::cout << _OPTION_STR(name);
    value.print();
}

_INTERNAL void OptionPath::set(std::string fp) {
    value.setCurrentValue(fp);
}

_INTERNAL std::string OptionPath::getCurrentValue() const {
    return value.curr_str;
}

#undef _OPTION
