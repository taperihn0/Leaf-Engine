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

#pragma once

#include "UtilsCommon.hpp"
#include "TrainEntry.hpp"
#include "StaticEval.hpp"
#include "Memory.hpp"

namespace Utils {

_INTERNAL bool packedPos2TrainingEntryFile(std::ifstream& pck_input, 
                                           std::ofstream& train_data_output, 
                                           TrainingDataEntry::Result8b game_result) 
{
    ASSERT(pck_input and train_data_output, "Given files are not valid");

    std::vector<ExtPackedPosition> packs = ExtPackedPosition::fullRead(pck_input);

    for (ExtPackedPosition& pack : packs) {
        Position full_position = ExtPackedPosition::unpacked(pack);
        Score white_score = StaticEval::staticEval(full_position);
        TrainingDataEntry entry(pack, white_score, game_result);
        
        if (!TrainingDataEntry::write(train_data_output, entry)) {
            ASSERT(false, "Failed to write binary string buffer to given file");
            return false;
        }
    }
    
    return train_data_output.good();
}

} // namespace Utils
