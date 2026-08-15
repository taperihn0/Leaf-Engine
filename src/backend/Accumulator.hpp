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

#pragma once

#include "PackedNetwork.hpp"
#include "Position.hpp"

namespace nn {

static constexpr size_t NetworkAccumulatorSizePerSide = NetworkHiddenLayerSize;

class alignas(CachelineSize) Accumulator {
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
    _NODISCARD static uint16_t featureIndex(Square sq, 
                                            Piece::enumType piece_type, 
                                            enumColor side);
    _NODISCARD static uint16_t featureIndex(enumColor perspective, 
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
                 const uint16_t* _RESTRICT side_active_features,
                 size_t side_active_features_cnt);

    void update(const PackedNeuralNetwork& network,
                const Accumulator* _RESTRICT prev_accum,
                const uint16_t* _RESTRICT added_features,
                size_t added_features_cnt,
                const uint16_t* _RESTRICT removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void update(const int16_t* _RESTRICT weights,
                const Accumulator* _RESTRICT prev_accum,
                const uint16_t* _RESTRICT added_features,
                size_t added_features_cnt,
                const uint16_t* _RESTRICT removed_features,
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
    AccumulatorCache() = default;
    
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
