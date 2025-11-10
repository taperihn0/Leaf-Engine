#include "Collector.hpp"

namespace Utils
{

void DataCollector::startTournament(size_t games_count, std::string file, SearchLimits limits) {
    std::ofstream output(file, std::ios::binary | std::ios::out);

    if (!output) {
        ASSERT(false, "Failed to open file " + file);
        return;
    }

    std::vector<PackedPosition> positions;
    positions.reserve(MaxGameMoves);

    for (size_t i = 1; i <= games_count; i++) {
        _match.start(positions, limits);

        for (size_t j = 0; j < positions.size(); j++) {
            const PackedPosition* packed_position = positions.data() + j;

            ASSERT(output.write(reinterpret_cast<const char*>(packed_position), sizeof(PackedPosition)), 
                   "Failed to write packed position");
        }

        positions.clear();
    }
}

} // namespace Utils
