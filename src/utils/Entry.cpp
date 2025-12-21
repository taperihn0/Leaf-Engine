#include "Entry.hpp"

#include <sstream>

namespace Utils {

TrainingDataEntry::TrainingDataEntry(const PackedPosition& packed, Score white_score, Result8b result)
    : _packed_pos(packed)
    , _game_details{ white_score, result, packed.getKingSquare(), packed.getOppKingSquare() }
{}

TrainingDataEntry::TrainingDataEntry(const ExtPackedPosition& packed, Score white_score, Result8b result)
    : TrainingDataEntry(PackedPosition::fromExt(packed), white_score, result)
{}

bool TrainingDataEntry::write(std::ostream& output, const TrainingDataEntry& entry) {
    assert(output);

    std::ostringstream buff(std::ios::binary);

    if (!PackedPosition::writeStatic(buff, entry._packed_pos)) {
        ASSERT(false, "Failed to write packed position of training entry to buffer");
        return false;
    }

    if (!output.write(reinterpret_cast<const char*>(&entry._game_details), sizeof(PackedPosInfo))) {
        ASSERT(false, "Failed to write training data entry to the output file");
        return false;
    }

    return output.good();
}

bool TrainingDataEntry::read(std::istream& input, TrainingDataEntry& entry) {
    assert(input);

    if (!PackedPosition::readStatic(input, entry._packed_pos)) {
        ASSERT(false, "Failed to read packed position of training entry from file");
        return false;
    }

    if (!input.read(reinterpret_cast<char*>(&entry._game_details), sizeof(PackedPosInfo))) {
        ASSERT(false, "Failed to read training data game info from file");
        return false;
    }

    return input.good();
}

} // Utils
