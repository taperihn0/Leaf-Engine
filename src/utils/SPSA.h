#include "UtilsCommon.hpp"
#include "SelfGame.hpp"

namespace Utils {

class SPSA_Tuning {
public:
    SPSA_Tuning() = default;
    void run();
private:
    float match(SelfGame& judge, SearchLimits limits);
};

}
