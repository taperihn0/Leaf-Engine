#pragma once

#include "PackedNetwork.hpp"
#include "Position.hpp"

namespace nn {

static constexpr size_t NetworkAccumulatorSize = NetworkHiddenLayerSize;

class alignas(CACHELINE_SIZE) Accumulator {
public:
    Accumulator() = default;

    static int activeFeatureIndex(enumColor perspective, 
                                  Square sq, 
                                  Piece piece);

    static int activeFeatureIndex(enumColor perspective, 
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

    const int16_t* getValues(enumColor side) const;
private:
    int16_t _values[2][NetworkHiddenLayerSize];
};

} // namespace nn
