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

#include "TrainEntry.hpp"

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

    if (!PackedPosition::writeStatic(output, entry._packed_pos)) {
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

    if (!PackedPosition::readStatic(input, entry._packed_pos))
        return false;

    if (!input.read(reinterpret_cast<char*>(&entry._game_details), sizeof(PackedPosInfo))) {
        ASSERT(false, "Failed to read training data game info from file");
        return false;
    }

    return input.good();
}

Score TrainingDataEntry::getWhiteScore() const {
    return _game_details.white_score;
}

TrainingDataEntry::Result8b TrainingDataEntry::getGameResult() const {
    return _game_details.result;
}

PackedPosition TrainingDataEntry::getPosition() const {
    return _packed_pos;
}

} // Utils
