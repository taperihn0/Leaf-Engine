#pragma once

#include "UtilsCommon.hpp"
#include "Entry.hpp"
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
