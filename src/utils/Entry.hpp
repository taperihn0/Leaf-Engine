#pragma once

#include "PackedPosition.hpp"

namespace Utils {

class TrainingDataEntry {
public:
    TrainingDataEntry() = default;

    enum Result8b : uint8_t {
        DRAW      = 0x0,
        WHITE_WIN = 0x1,
        BLACK_WIN = 0x2,
    };

    TrainingDataEntry(const PackedPosition& packed, Score white_score, Result8b result);

    static bool write(std::ostream& output, const TrainingDataEntry& entry);

    static bool read(std::istream& input, TrainingDataEntry& entry);
private:
    PackedPosition _packed_pos;

#pragma pack(push, 1)
    struct GameDetails {
        Result8b result;
        Score    white_score;
    };
#pragma pack(pop)

    static_assert(sizeof(GameDetails) == 3);

    GameDetails _game_details;
};

} // namespace Utils
