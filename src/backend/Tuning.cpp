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
    _addr["HalfMovesEvalLimit"] = &HalfMovesEvalLimit;
    _addr["NullVerifyDepth"] = &NullVerifyDepth;
    _addr["MoveCheckExtensionRate"] = &MoveCheckExtensionRate ;
    _addr["MoveCheckExtensionDiv"] = &MoveCheckExtensionDiv ;
    _addr["ImprovingExtensionMateRate"] = &ImprovingExtensionMateRate ;
    _addr["MateThreadFracExtensionRate"] = &MateThreadFracExtensionRate;
    _addr["MateThreadFracExtensionDiv"] = &MateThreadFracExtensionDiv;
    _addr["MaxMoveExtensionRate"] = &MaxMoveExtensionRate;
    _addr["MaxMoveExtensionDiv"] = &MaxMoveExtensionDiv;
    _addr["NullVerifyDepthMult"] = &NullVerifyDepthMult;
    _addr["ExtensionDepth"] = &ExtensionDepth;
    _addr["SingularDepth"] = &SingularDepth;
    _addr["SingularDepthMargin"] = &SingularDepthMargin;
    _addr["SingularExtensionRate"] = &SingularExtensionRate;
    _addr["SingularBetaDepthMult"] = &SingularBetaDepthMult;
    _addr["SingularDepthMult"] = &SingularDepthMult;
    _addr["SingularDepthBase"] = &SingularDepthBase;
    _addr["SingularBetaExtensionRate"] = &SingularBetaExtensionRate;
    _addr["TablebaseProbeDepth"] = &TablebaseProbeDepth;
    _addr["TablebasePieceCountLimit"] = &TablebasePieceCountLimit;
    _addr["TablebaseWinScore"] = &TablebaseWinScore;
    _addr["TablebasePieceDiffMult"] = &TablebasePieceDiffMult;
    _addr["TablebaseScoreScale"] = &TablebaseScoreScale;
    _addr["KnightCapturedScore"] = &KnightCapturedScore;
    _addr["BishopCapturedScore"] = &BishopCapturedScore;
    _addr["ToKnightPromoScore"] = &ToKnightPromoScore;
    _addr["ToBishopPromoScore"] = &ToBishopPromoScore;
    _addr["ToRookPromoScore"] = &ToRookPromoScore;
    _addr["ToQueenPromoScore"] = &ToQueenPromoScore;
    _addr["AspirationSearchDepth"] = &AspirationSearchDepth;
    _addr["AspirationFirstWindow"] = &AspirationFirstWindow;
    _addr["AspirationUnstableFactor"] = &AspirationUnstableFactor;
    _addr["AspirationDepthRate"] = &AspirationDepthRate;
    _addr["AspirationMaxDepthInfl"] = &AspirationMaxDepthInfl;
    _addr["AspirationWindowScoreDiv"] = &AspirationWindowScoreDiv;
    _addr["AspirationMaxWindow"] = &AspirationMaxWindow;
    _addr["AspirationCount"] = &AspirationCount;
    _addr["AspirationWidenRate"] = &AspirationWidenRate;
};

void* TunableParametersMap::getAddressOf(std::string str) {
    void* base;

    try {
        base = _addr.at(str);
    } catch (std::out_of_range&) {
        ASSERT(false, "Invalid parameter option: " + str);
        return nullptr;
    }

    return base;
}

TunableParametersMap GlobParamMapping;

#endif
