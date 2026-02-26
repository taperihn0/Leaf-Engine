#include "Tuning.hpp"
#include "MoveOrder.hpp"
#include "Search.hpp"

#if defined(_ENABLE_TUNING)

void TunableParametersMap::createMapping() {
    _addr["MaxQuietsHistoryPow"] = &MaxQuietsHistoryPow;
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
    _addr["NNEvalScale"]         = &NNEvalScale;
    _addr["NullDepth"]           = &NullDepth;
    _addr["ImprovingRate"]       = &ImprovingRate;
    _addr["RfpImprovingSink"]    = &RfpImprovingSink;
    _addr["NullMargin"]          = &NullMargin;
    _addr["NullImprovingSink"]   = &NullImprovingSink;
    _addr["DynImprovementDepth"] = &DynImprovementDepth;
    _addr["TTEvalCorrRate"]      = &TTEvalCorrRate;
    _addr["NullDiffScale"]       = &NullDiffScale;
    _addr["NextDepthTimeRed"]    = &NextDepthTimeRed;  
    _addr["UnstableMatMargin"]   = &UnstableMatMargin;
    _addr["UnstableMultMargin"]  = &UnstableMultMargin;
    _addr["MinTimeBranchFactor"] = &MinTimeBranchFactor;
    _addr["MaxTimeBranchFactor"] = &MaxTimeBranchFactor;
    _addr["KnightCapturedScore"] = &KnightCapturedScore;
    _addr["BishopCapturedScore"] = &BishopCapturedScore;
    _addr["ToKnightPromoScore"]  = &ToKnightPromoScore;
    _addr["ToBishopPromoScore"]  = &ToBishopPromoScore;
    _addr["ToRookPromoScore"]    = &ToRookPromoScore;
    _addr["ToQueenPromoScore"]   = &ToQueenPromoScore;
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
