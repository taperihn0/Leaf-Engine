#pragma once

#include "PackedNetwork.hpp"
#include "Position.hpp"

namespace nn {

static constexpr size_t NetworkAccumulatorSizePerSide = NetworkHiddenLayerSize;

class alignas(CACHELINE_SIZE) Accumulator {
public:
    _INLINE Accumulator() = default;
    _INLINE Accumulator(const PackedNeuralNetwork& network, 
                        const Position& pos) 
    { refresh(network, pos); }

    Accumulator(const Accumulator&) = delete;
    Accumulator(Accumulator&&) = delete;

    Accumulator& operator=(const Accumulator&) = delete;
    Accumulator& operator=(Accumulator&&) = delete;

    _INLINE bool operator==(const Accumulator& accum) const { return _values == accum._values; }
    _INLINE bool operator!=(const Accumulator& accum) const { return !(*this == accum); }

    template <enumColor Perspective>
    _NODISCARD static int featureIndex(Square sq, 
                                       Piece::enumType piece_type, 
                                       enumColor side);
    _NODISCARD static int featureIndex(enumColor perspective, 
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

    _NODISCARD const array1d<int16_t, NetworkAccumulatorSizePerSide>& getValues(enumColor side) const;

#if defined(_VERIFY_NN)
    static bool verify(const Accumulator& accum, const Position& pos);
#endif

private:
    array2d<int16_t, 2, NetworkAccumulatorSizePerSide> _values;
};

struct FeatureData {
    _INLINE FeatureData() = default;
    _INLINE FeatureData(Square pc_sq, Piece::enumType pc_type, enumColor pc_color)
        : sq(pc_sq)
        , piece_type(pc_type)
        , side(pc_color)
    {}

    _NODISCARD _INLINE bool operator==(const FeatureData& f) const {
        return sq == f.sq and piece_type == f.piece_type and side == f.side;
    };

    Square          sq;
    Piece::enumType piece_type = Piece::enumType::NONE;
    enumColor       side;
};

struct AccumulatorCache {
    _INLINE AccumulatorCache() = default;
    _NODISCARD _INLINE bool isDirty() const { return dirty;  }
    _NODISCARD _INLINE bool isClean() const { return !dirty; }
    _INLINE void markClean() { dirty = false; }
    _INLINE void markDirty() { dirty = true;  }
    void clearBuffers();

    Accumulator             accum;
    array1d<FeatureData, 2> added_features;
    array1d<FeatureData, 2> removed_features;
    size_t                  added_features_cnt   = 0;
    size_t                  removed_features_cnt = 0;
    bool                    dirty                = false;
};

} // namespace nn
