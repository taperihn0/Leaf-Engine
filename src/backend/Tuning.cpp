#include "Tuning.hpp"
#include "MoveOrder.hpp"
#include "Search.hpp"

#if defined(_ENABLE_TUNING)

void TunableParametersMap::createMapping() {
    _addr["IidDepth"] = &IidDepth;
    _addr["IidDepthDiv"] = &IidDepthDiv;
    _addr["RfpDepth"] = &RfpDepth;
    _addr["RazorDepth"] = &RazorDepth;
    _addr["FutilityDepth"] = &FutilityDepth;
    _addr["LmrDepth"] = &LmrDepth;
    _addr["NullDiffScale"] = &NullDiffScale;
    _addr["NullReduction"] = &NullReduction;
    _addr["NullDepth"] = &NullDepth;
    _addr["LmrMoveCount"] = &LmrMoveCount;
    _addr["RazorMultDelta"] = &RazorMultDelta;
    _addr["RfpMultDelta"] = &RfpMultDelta;
    _addr["FutilityDelta"] = &FutilityDelta;
    _addr["FutilityMoveCount"] = &FutilityMoveCount;
    _addr["RazorBaseDelta"] = &RazorBaseDelta;
    _addr["QMaterialDelta"] = &QMaterialDelta;
    _addr["QProbeDepth"] = &QProbeDepth;
    _addr["NNEvalScale"] = &NNEvalScale;
    _addr["ImprovingRate"] = &ImprovingRate;
    _addr["RfpImprovingSink"] = &RfpImprovingSink;
    _addr["NullMargin"] = &NullMargin;
    _addr["NullImprovingSink"] = &NullImprovingSink;
    _addr["DynImprovementDepth"] = &DynImprovementDepth;
    _addr["TTEvalCorrRate"] = &TTEvalCorrRate;
    _addr["NextDepthTimeRed"] = &NextDepthTimeRed;
    _addr["UnstableMatMargin"] = &UnstableMatMargin;
    _addr["UnstableMultMargin"] = &UnstableMultMargin;
    _addr["MinTimeBranchFactor"] = &MinTimeBranchFactor;
    _addr["MaxTimeBranchFactor"] = &MaxTimeBranchFactor;
    _addr["ContemptDiv"] = &ContemptDiv;
    _addr["ImprovingExtensionRate"] = &ImprovingExtensionRate;
    _addr["NotPvNodeReduction"] = &NotPvNodeReduction;
    _addr["CheckReduction"] = &CheckReduction;
    _addr["ExtensionReduction"] = &ExtensionReduction;
    _addr["PawnMoveReduction"] = &PawnMoveReduction;
    _addr["ImprovingReductionRate"] = &ImprovingReductionRate;
    _addr["MoveCountReductionRate"] = &MoveCountReductionRate;
    _addr["MoveCountReductionDiv"] = &MoveCountReductionDiv;
    _addr["HashCapReduction"] = &HashCapReduction;
    _addr["KillerMoveReduction"] = &KillerMoveReduction;
    _addr["MoveScoreReductionRate"] = &MoveScoreReductionRate;
    _addr["TotalReductionRate"] = &TotalReductionRate;
    _addr["KnightCapturedScore"] = &KnightCapturedScore;
    _addr["BishopCapturedScore"] = &BishopCapturedScore;
    _addr["ToKnightPromoScore"]  = &ToKnightPromoScore;
    _addr["ToBishopPromoScore"]  = &ToBishopPromoScore;
    _addr["ToRookPromoScore"]    = &ToRookPromoScore;
    _addr["ToQueenPromoScore"]   = &ToQueenPromoScore;
    _addr["MaxQuietsHistoryPow"] = &MaxQuietsHistoryPow;
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
