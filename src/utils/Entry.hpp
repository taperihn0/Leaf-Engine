#pragma once

#include "PackedPosition.hpp"

namespace Utils {

class TrainingDataEntry {
public:
    TrainingDataEntry() = default;

    enum Result8b : uint8_t {
        BLACK_WIN = 0,
        DRAW      = 1,
        WHITE_WIN = 2,
    };

    TrainingDataEntry(const PackedPosition& packed, Score white_score, Result8b result);
    TrainingDataEntry(const ExtPackedPosition& packed, Score white_score, Result8b result);

    static bool write(std::ostream& output, const TrainingDataEntry& entry);
    static bool read(std::istream& input, TrainingDataEntry& entry);

    PackedPosition getPosition() const;
private:
#pragma pack(push, 1)
    struct PackedPosInfo {
        Score     white_score;
        Result8b  result;
        Square    king_sq;
        Square    opp_king_sq;
        std::byte extra[3];
    };
#pragma pack(pop)

    static_assert(sizeof(PackedPosInfo) == 8);

    PackedPosition _packed_pos;
    PackedPosInfo  _game_details;
};

} // namespace Utils
