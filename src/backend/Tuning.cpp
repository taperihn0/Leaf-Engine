#include "Tuning.hpp"
#include "MoveOrder.hpp"
#include "Search.hpp"

#if defined(_ENABLE_TUNING)

void TunableParametersMap::createMapping() {
    _addr["MaxQuietsHistoryPow"] = &MaxQuietsHistoryPow;
    _addr["CheckNodeCount"]      = &CheckNodeCount;
    _addr["IidDepth"]            = &IidDepth;
    _addr["IidDepthDiv"]         = &IidDepthDiv;
    _addr["RfpDepth"]            = &RfpDepth;
    _addr["RazorDepth"]          = &RazorDepth;
    _addr["FutilityDepth"]       = &FutilityDepth;
    _addr["LmrDepth"]            = &LmrDepth;
    _addr["NullReduction"]       = &NullReduction;
    _addr["LmrMoveCount"]        = &LmrMoveCount;
    _addr["RazorMultDelta"]      = &RazorMultDelta;
    _addr["RfpMultDelta"]        = &RfpMultDelta;
    _addr["FutilityMoveCount"]   = &FutilityMoveCount; 
    _addr["FutilityDelta"]       = &FutilityDelta;
    _addr["RazorBaseDelta"]      = &RazorBaseDelta;
    _addr["QMaterialDelta"]      = &QMaterialDelta;
    _addr["QProbeDepth"]         = &QProbeDepth;
};

void* TunableParametersMap::getAddressOf(std::string str) {
    void* base;

    try {
        base = _addr.at(str);
    } catch (std::out_of_range&) {
        ASSERT(false, "Invalid parameter option");
        return nullptr;
    }

    return base;
}

TunableParametersMap GlobParamMapping;

#endif
