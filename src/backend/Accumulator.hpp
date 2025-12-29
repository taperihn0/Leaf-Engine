#pragma once

#include "PackedNetwork.hpp"
#include "Position.hpp"

namespace nn {

static constexpr size_t NetworkAccumulatorSize = NetworkHiddenLayerSize;

class alignas(CACHELINE_SIZE) Accumulator {
public:
    Accumulator() = default;

    bool operator==(const Accumulator& accum) const ;
    INLINE bool operator!=(const Accumulator& accum) const { return !(*this == accum); }

    template <enumColor Perspective>
    static int featureIndex(Square sq, Piece::enumType piece_type, enumColor side);
    static int featureIndex(enumColor perspective, 
                            Square sq, 
                            Piece::enumType piece_type, 
                            enumColor side);

    void refresh(const int16_t* biases, 
                 const int16_t* weights, 
                 const Position& pos);

    void refresh(const int16_t* biases, 
                 const int16_t* weights, 
                 enumColor side, 
                 int* side_active_features,
                 size_t side_active_features_cnt);

    void update(const int16_t* weights,
                const Accumulator* prev_accum,
                int* added_features,
                size_t added_features_cnt,
                int* removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void clear(enumColor side);

    const int16_t* getValues(enumColor side) const;
private:
    int16_t _values[2][NetworkHiddenLayerSize];
};

} // namespace nn
