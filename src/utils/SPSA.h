#include "UtilsCommon.hpp"
#include "SelfGame.hpp"

namespace Utils {

class SPSA_Tuning {
public:
    SPSA_Tuning();
private:
    float match(SelfGame& judge, SearchLimits limits);
};

}
