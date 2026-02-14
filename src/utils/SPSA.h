#include "UtilsCommon.hpp"

namespace Utils {

class SPSA_Tuning {
public:
    SPSA_Tuning() = default;
    void run();
private:
    float match(SearchLimits limits,
                std::istream& engine_os0, std::ostream& engine_is0,
                std::istream& engine_os1, std::ostream& engine_is1);

    void sentPosition(const std::string& start_fen, 
                      const FullInfoRecord& record,
                      std::istream& engine_os, std::ostream& engine_is);

    Move32b getPlayerMove(SearchLimits limits, 
                          const Position& pos,
                          std::istream& engine_os, std::ostream& engine_is, 
                          Score& score);
};

}
