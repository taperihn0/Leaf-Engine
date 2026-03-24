#include "UtilsCommon.hpp"
#include "frontend/Options.hpp"
#include "Opening.hpp"

#include <vector>
#include <fstream>
#include <thread>

namespace Utils {

enum enumLogLabel : uint16_t {
    LOG_NO_LABEL = 0,
    LOG_DEBUG    = 1,
    LOG_INFO     = 2,
    LOG_ENGINE_0 = 4,
    LOG_ENGINE_1 = 8,
    LOG_THREAD_1 = 16,
    LOG_THREAD_2 = 32,
    LOG_THREAD_3 = 64,
    LOG_THREAD_4 = 128,
    LOG_THREAD_5 = 256,
    LOG_THREAD_6 = 512,
    LOG_THREAD_7 = 1024,
    LOG_THREAD_8 = 2048,
};

class SPSA_Tuning {
public:
    SPSA_Tuning() = default;
    void start(uint thread_count, const std::string& spsa_log);

    static constexpr int ThreadLimit = 8;
private:
    struct SPSA_Parameter {
        std::string name;
        double      min, max;
        double      value;
        double      c;
        double      r;
        double      a;
        double      ak;
        double      ck;
        double      delta;
    };

    struct SPSA_PackedParameter {
        const std::string* name;
        double             value;
    };

    void startThread(std::vector<SPSA_Parameter>& theta, 
                     std::ofstream& log_file, 
                     SearchLimits limits,
                     uint id);

    void tune(std::vector<SPSA_Parameter>& params,
              std::vector<SPSA_PackedParameter>& theta_plus,
              std::vector<SPSA_PackedParameter>& theta_minus,
              uint n, SearchLimits limits,
              std::istream& engine_os0, std::ostream& engine_is0,
              std::istream& engine_os1, std::ostream& engine_is1,
              std::ofstream& log_file,
              uint id);

    void writeCheckpoint(std::ofstream& file, 
                         std::vector<SPSA_Parameter>& theta,
                         uint k);

    void applyOptions(const std::vector<SPSA_PackedParameter>& tunable_options,
                      std::istream& engine_os, std::ostream& engine_is,
                      enumLogLabel ret_msg_label);

    int  match(SearchLimits limits,
               std::istream& engine_os0, std::ostream& engine_is0,
               std::istream& engine_os1, std::ostream& engine_is1,
               std::string& info,
               uint id);

    void sentPosition(const std::string& start_fen, 
                      const FullInfoRecord& record,
                      std::istream& engine_os, std::ostream& engine_is,
                      enumLogLabel ret_msg_label);

    Move32b getPlayerMove(SearchLimits limits, 
                          const Position& pos,
                          std::istream& engine_os, std::ostream& engine_is, 
                          Score& score,
                          enumLogLabel ret_msg_label);

    OpeningSuite _openings;
};

}
