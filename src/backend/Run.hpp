#pragma once

#include "Common.hpp"

class EngineRun {
public:
    virtual void init() = 0;
    virtual void finish() = 0;
};
