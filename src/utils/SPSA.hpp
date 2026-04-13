#include "UtilsCommon.hpp"
#include "frontend/Options.hpp"
#include "Opening.hpp"
#include "UtilsCommon.hpp"

#include <vector>
#include <fstream>
#include <thread>

namespace Utils {

class SPSA_Tuning {
public:
    SPSA_Tuning() = default;
    void start(uint thread_count, const std::string& spsa_log);
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

    int match(SearchLimits limits,
              std::istream& engine_os0, std::ostream& engine_is0,
              std::istream& engine_os1, std::ostream& engine_is1,
              std::shared_ptr<Game::Result> result,
              uint id);

    static constexpr bool _EnableSelfPlayLog = true;                      
    OpeningSuite _openings;
};

}
