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

#include "Options.hpp"
#include "TranspositionTable.hpp"
#include "Common.hpp"
#include "PackedNetwork.hpp"
#include "Tuning.hpp"

#if defined(_USE_EMBEDDED_NEURAL_NET)
static std::filesystem::path DefaultNeuralNetOptionPath = "<embedded:" DEFAULT_NEURAL_NET_FILE_NAME ">";
#else
static std::filesystem::path DefaultNeuralNetOptionPath = nn::DefaultNetworkFile;
#endif

namespace opt {
    
Options::Options() 
    // --- Regular parameters ---
    : hash_opt(SpinType<ll>(tt::DefaultTTSizeMb / 1_MB, 1, 512)),
      clear_hash_opt(),
      syzygy_opt(    "SyzygyPath",    StringType("<empty>")),
      neural_net_opt("NeuralNetPath", StringType(DefaultNeuralNetOptionPath.string()))
{
#if defined(_ENABLE_TUNING)
    for (const auto& cluster : GlobParamMapping) {
        const std::string& name = cluster.first;
        const TunableParametersMap::ParameterInfo& parameter = cluster.second;

        try {
            tunable_params_opt.emplace_back(
                SpinType<double>(*reinterpret_cast<const int32_t*>(parameter.addr), 
                                 parameter.mi, parameter.ma), 
                name, parameter.step_rate);
        }
        catch (const std::logic_error& e) {
            std::cout << "Failed to initialize tunable parameter of name: <" << name << ">" << std::endl;
            throw e;
        }
    }
#endif
}

} // namespace opt
