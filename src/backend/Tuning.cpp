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
    _addr["QuietNotPvNodeReduction"] = &QuietNotPvNodeReduction;
    _addr["QuietCutNodeReduction"] = &QuietCutNodeReduction;
    _addr["QuietCheckReduction"] = &QuietCheckReduction;
    _addr["QuietExtensionReduction"] = &QuietExtensionReduction;
    _addr["QuietPawnMoveReduction"] = &QuietPawnMoveReduction;
    _addr["QuietImprovingReductionRate"] = &QuietImprovingReductionRate;
    _addr["QuietHashCapReduction"] = &QuietHashCapReduction;
    _addr["QuietKillerMoveReduction"] = &QuietKillerMoveReduction;
    _addr["QuietMoveScoreReductionRate"] = &QuietMoveScoreReductionRate;
    _addr["QuietMoveScoreReductionDiv"] = &QuietMoveScoreReductionDiv;
    _addr["QuietTotalReductionRate"] = &QuietTotalReductionRate;
    _addr["CaptureNotPvNodeReduction"] = &CaptureNotPvNodeReduction;
    _addr["CaptureCutNodeReduction"] = &CaptureCutNodeReduction;
    _addr["CaptureCheckReduction"] = &CaptureCheckReduction;
    _addr["CaptureHashCapReduction"] = &CaptureHashCapReduction;
    _addr["CaptureKillerMoveReduction"] = &CaptureKillerMoveReduction;
    _addr["CaptureMoveScoreReductionDiv"] = &CaptureMoveScoreReductionDiv;
    _addr["CaptureExtensionReduction"] = &CaptureExtensionReduction;
    _addr["CaptureImprovingReductionRate"] = &CaptureImprovingReductionRate;
    _addr["CaptureTotalReductionRate"] = &CaptureTotalReductionRate;
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
