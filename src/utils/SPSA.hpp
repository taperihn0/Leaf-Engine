/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "UtilsCommon.hpp"
#include "frontend/Options.hpp"
#include "Opening.hpp"
#include "UtilsCommon.hpp"
#include "Process.hpp"

#include <vector>
#include <fstream>
#include <thread>

namespace Utils {

class SPSA_Tuning {
public:
    SPSA_Tuning() = default;
    void start(uint thread_count, const std::filesystem::path& spsa_log);
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
              EngineProcess& engine0,
              EngineProcess& engine1,
              std::ofstream& log_file,
              uint id);

    void writeCheckpoint(std::ofstream& file, 
                         std::vector<SPSA_Parameter>& theta,
                         uint k);

    void applyOptions(const std::vector<SPSA_PackedParameter>& tunable_options,
                      EngineProcess& engine,
                      enumLogLabel ret_msg_label);

    int match(SearchLimits limits,
              EngineProcess& engine0,
              EngineProcess& engine1,
              std::shared_ptr<Game::Result> result,
              uint id,
              enumLogLabel thread_label);

    static constexpr bool _EnableSelfPlayLog = true;
    OpeningSuite _openings;
};

} // namespace Utils
