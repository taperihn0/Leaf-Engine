#pragma once

#include "backend/Common.hpp"

enum class OptionValueType {
    TYPE_SPIN = 0,
    TYPE_BUTTON = 1
};

struct OptionType {
    virtual void print() const = 0;
};

struct SpinType : public OptionType {
    using int_t = int;

    SpinType(int_t def, int_t ma, int_t mi);
    void setCurrent(int_t val);
    void print() const override;

    int_t default_value;
    int_t current_value;
    int_t max_value;
    int_t min_value;
};

struct ButtonType : public OptionType {
    using type = bool;
    void print() const override;
    type current_value;
};

struct Option {
    virtual void print() const = 0;
};

struct OptionHash : public Option {
    OptionHash(SpinType val);
    void print() const override;
    void set(SpinType::int_t val);
    SpinType::int_t getCurrentValue() const;

    SpinType value;
};

struct OptionClearHash : public Option {
    OptionClearHash() = default;
    void print() const override;

    ButtonType value;
};

_INTERNAL SpinType::SpinType(int_t def, int_t mi, int_t ma)
    : default_value(def)
    , current_value(0)
    , max_value(ma)
    , min_value(mi)
{
    setCurrent(def);
}

_INTERNAL void SpinType::setCurrent(int_t val) {
    current_value = std::clamp(val, min_value, max_value);
}

_INTERNAL void SpinType::print() const {
    std::cout << " type spin default " << default_value 
              << " min " << min_value
              << " max " << max_value << std::endl;
}

_INTERNAL void ButtonType::print() const {
    std::cout << " type button" << std::endl;
}

#define _OPTION(name) "option name " name

_INTERNAL OptionHash::OptionHash(SpinType val)
    : value(val) 
{}

_INTERNAL void OptionHash::set(SpinType::int_t val) {
    value.setCurrent(val);
}

_INTERNAL SpinType::int_t OptionHash::getCurrentValue() const {
    return value.current_value;
}

_INTERNAL void OptionHash::print() const {
    std::cout << _OPTION("Hash");
    value.print();
}

_INTERNAL void OptionClearHash::print() const {
    std::cout << _OPTION("Clear Hash");
    value.print();
}

#undef _OPTION
