#pragma once

#include "PackedNetwork.hpp"
#include "Position.hpp"

namespace nn {

static constexpr size_t NetworkAccumulatorSize = NetworkHiddenLayerSize;

class alignas(CACHELINE_SIZE) Accumulator {
public:
    Accumulator() = default;
    Accumulator(const Accumulator&) = delete;
    // just refresh the accumulator with the given 'network' and 'pos'
    Accumulator(const PackedNeuralNetwork& network, 
                const Position& pos);

    Accumulator& operator=(const Accumulator&) = delete;

    bool operator==(const Accumulator& accum) const ;
    INLINE bool operator!=(const Accumulator& accum) const { return !(*this == accum); }

    template <enumColor Perspective>
    static int featureIndex(Square sq, Piece::enumType piece_type, enumColor side);
    static int featureIndex(enumColor perspective, 
                            Square sq, 
                            Piece::enumType piece_type, 
                            enumColor side);

    void refresh(const PackedNeuralNetwork& network,
                 const Position& pos);

    void refresh(const int16_t* _RESTRICT biases, 
                 const int16_t* _RESTRICT weights, 
                 const Position& pos);

    void refresh(const int16_t* _RESTRICT biases, 
                 const int16_t* _RESTRICT weights, 
                 enumColor side, 
                 const int* _RESTRICT side_active_features,
                 size_t side_active_features_cnt);

    void update(const PackedNeuralNetwork& network,
                const Accumulator* _RESTRICT prev_accum,
                const int* _RESTRICT added_features,
                size_t added_features_cnt,
                const int* _RESTRICT removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void update(const int16_t* _RESTRICT weights,
                const Accumulator* _RESTRICT prev_accum,
                const int* _RESTRICT added_features,
                size_t added_features_cnt,
                const int* _RESTRICT removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void clear(enumColor side);

    const int16_t* getValues(enumColor side) const;

#if defined(_VERIFY_NN)
    static bool verify(const Accumulator& accum, const Position& pos);
#endif
private:
    int16_t _values[2][NetworkHiddenLayerSize];
};

struct FeatureData {
    FeatureData() = default;
    INLINE FeatureData(enumColor persp, Square pc_sq, Piece::enumType pc_type, enumColor pc_color)
        : perspective(persp)
        , sq(pc_sq)
        , piece_type(pc_type)
        , side(pc_color)
    {
        assert(sq.isValid());
    }

    enumColor       perspective;
    Square          sq;
    Piece::enumType piece_type = Piece::enumType::NONE;
    enumColor       side;
};

struct AccumulatorCache {
    bool isDirty() const;
    bool isClean() const;
    void markClean();
    void markDirty();
    void clearBuffers();

    Accumulator accum;
    FeatureData added_features[2][2];
    FeatureData removed_features[2][2];
    size_t      added_features_cnt   = 0;
    size_t      removed_features_cnt = 0;
    bool        dirty                = false;
};

} // namespace nn
