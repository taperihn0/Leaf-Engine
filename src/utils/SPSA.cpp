#include "SPSA.h"

namespace Utils {

SPSA_Tuning::SPSA_Tuning() {

}

INLINE float SPSA_Tuning::match(SelfGame& judge, SearchLimits limits) {
    const auto res = judge.start(limits);
    
    return isWhiteWin(res) ?  1 :
           isBlackWin(res) ? -1 :
                              0.5;
}

}
