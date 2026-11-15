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
    using value_type = PackedNeuralNetwork::value_type;
    static constexpr size_t MaxPerspectiveFeatureCount = 32;

    Accumulator() = default;
    _INLINE Accumulator(const PackedNeuralNetwork& network, 
                        const Position& pos) 
    { refresh(network, pos); }

    Accumulator(const Accumulator&) = delete;
    Accumulator(Accumulator&&) = delete;

    Accumulator& operator=(const Accumulator&) = delete;
    Accumulator& operator=(Accumulator&&) = delete;

    _INLINE bool operator==(const Accumulator& accum) const { return _persp_accum == accum._persp_accum; }
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

    void refresh(std::span<const value_type> biases, 
                 std::span<const value_type> weights, 
                 const Position& pos);

    void refresh(std::span<const value_type> biases, 
                 std::span<const value_type> weights, 
                 std::span<const uint16_t, MaxPerspectiveFeatureCount> side_active_features,
                 size_t side_active_features_cnt,
                 enumColor side);

    void update(const PackedNeuralNetwork& network,
                const Accumulator* _RESTRICT prev_accum,
                std::span<const uint16_t> added_features,
                size_t added_features_cnt,
                const std::span<const uint16_t> removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void update(const std::span<const value_type> weights,
                const Accumulator* _RESTRICT prev_accum,
                std::span<const uint16_t> added_features,
                size_t added_features_cnt,
                std::span<const uint16_t> removed_features,
                size_t removed_features_cnt,
                enumColor side);

    void clear(enumColor side);

    _NODISCARD std::span<const value_type, NetworkAccumulatorSizePerSide> 
    getValues(enumColor side) const;

#if defined(_VERIFY_NN)
    static bool verify(const Accumulator& accum, const Position& pos);
#endif

private:
    template <size_t PackedCount, typename Fn>
    void accumulatorPolicyUpdate(simd::Packed<value_type>* const _RESTRICT acc,
                                 const simd::Packed<value_type>* const _RESTRICT weights_base,
                                 Fn&& invoke_value);

    MultiArray<value_type, 2, NetworkAccumulatorSizePerSide> _persp_accum;
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

    _INLINE void clearBuffers() {
        added_features_cnt = 0;
        removed_features_cnt = 0;
        dirty = false;
    }

    Accumulator             accum;
    std::array<FeatureData, 2> added_features;
    std::array<FeatureData, 2> removed_features;
    size_t                  added_features_cnt   = 0;
    size_t                  removed_features_cnt = 0;
    bool                    dirty                = false;
};

} // namespace nn
