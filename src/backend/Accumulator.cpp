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

#include "Accumulator.hpp"
#include "Memory.hpp"

namespace nn {

using value_type = Accumulator::value_type;

template <enumColor Perspective>
uint16_t Accumulator::featureIndex(Square sq, 
                                   Piece::enumType piece_type, 
                                   enumColor side) 
{
    if constexpr (Perspective == BLACK) {
        return static_cast<uint16_t>(!side) * 64 * 6 
            + pc::value(piece_type) * 64 
            + static_cast<uint16_t>(sq::verticalFlip(sq));
    }

    return static_cast<uint16_t>(side) * 64 * 6 
        + pc::value(piece_type) * 64 
        + static_cast<uint16_t>(sq);
}

uint16_t Accumulator::featureIndex(enumColor perspective, 
                                   Square sq, 
                                   Piece::enumType piece_type, 
                                   enumColor side) 
{
    return perspective == BLACK ? featureIndex<BLACK>(sq, piece_type, side)
                                : featureIndex<WHITE>(sq, piece_type, side);
}

void Accumulator::refresh(const PackedNeuralNetwork& network,
                          const Position& pos) 
{
    refresh(network.getLayerBiases(0),
            network.getLayerWeights(0),
            pos);
}

void Accumulator::refresh(std::span<const value_type> biases, 
                          std::span<const value_type> weights, 
                          const Position& pos) 
{   
    assert(!biases.empty());
    assert(!weights.empty());

    MultiArray<uint16_t, 2, MaxPerspectiveFeatureCount> side_active_features;

    size_t active_features_cnt = 0;

    for (Piece::enumType piece_type : Piece::PieceTypeList) {
        for (enumColor side : { WHITE, BLACK }) {
            BitBoard bb = pos.get(piece_type, side);

            while (bb) {
                const Square sq = static_cast<Square>(bb.dropForward());
                
                side_active_features[WHITE][active_features_cnt] = featureIndex<WHITE>(sq, piece_type, side);
                side_active_features[BLACK][active_features_cnt++] = featureIndex<BLACK>(sq, piece_type, side);
                
                assert(active_features_cnt <= 32);
            }
        }
    }

    for (enumColor persp : { WHITE, BLACK }) {
        refresh(biases,
                weights, 
                side_active_features[persp], 
                active_features_cnt,
                persp);
    }
}

void Accumulator::refresh(std::span<const value_type> biases, 
                          std::span<const value_type> weights, 
                          std::span<const uint16_t, MaxPerspectiveFeatureCount> side_active_features,
                          size_t side_active_features_cnt,
                          enumColor side)
{
    assert(!biases.empty());
    assert(!weights.empty());
    assert(!side_active_features.empty());

#ifndef _NN_NO_SIMD // Use SIMD Extensions

    static constexpr int PackedCount = NetworkAccumulatorSizePerSide / simd::Packed<value_type>::count();
    static_assert(NetworkAccumulatorSizePerSide % PackedCount == 0);

    auto* const _RESTRICT side_acc = std::assume_aligned<CachelineSize>(
        reinterpret_cast<simd::Packed<value_type>*>(_persp_accum[side].data())
    );
    const auto* const _RESTRICT biases_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(biases.data())
    );
    const auto* const _RESTRICT weights_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(weights.data())
    );

    assert(reinterpret_cast<ptrdiff_t>(side_acc) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(biases_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    std::copy_n(biases_base, PackedCount, side_acc);

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const size_t base_offset = side_active_features[i] * PackedCount;

        for (size_t j = 0; j < PackedCount; j++) {
            const auto add = side_acc[j].aligned_load_this() + weights_base[base_offset + j].aligned_load_this();
            add.aligned_store(side_acc + j);
        }
    }
    
#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _persp_accum[side][i] = biases[i];
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const size_t base_offset = side_active_features[i] * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _persp_accum[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::update(const PackedNeuralNetwork& network,
                         const Accumulator* _RESTRICT prev_accum,
                         std::span<const uint16_t> added_features,
                         size_t added_features_cnt,
                         const std::span<const uint16_t> removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    update(network.getLayerWeights(0),
           prev_accum, 
           added_features,
           added_features_cnt,
           removed_features,
           removed_features_cnt,
           side);
}

template <size_t PackedCount, typename Fn>
void Accumulator::accumulatorPolicyUpdate(simd::Packed<value_type>* const _RESTRICT acc,
                                          const simd::Packed<value_type>* const _RESTRICT weights_base,
                                          Fn&& invoke_value) 
{
    for (auto i = 0uz; i < PackedCount; i++) {
        invoke_value(acc[i], weights_base[i]).aligned_store(acc + i);
    }
}

void Accumulator::update(const std::span<const value_type> weights,
                         const Accumulator* _RESTRICT prev_accum,
                         std::span<const uint16_t> added_features,
                         size_t added_features_cnt,
                         std::span<const uint16_t> removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    assert(!weights.empty());
    assert(prev_accum != nullptr);
    assert(!added_features.empty());
    assert(!removed_features.empty());
    assert(this != prev_accum);

#ifndef _NN_NO_SIMD // Use SIMD Extensions

    static constexpr int PackedCount = NetworkAccumulatorSizePerSide / simd::Packed<value_type>::count();
    static_assert(NetworkAccumulatorSizePerSide % PackedCount == 0);

    auto* const _RESTRICT side_acc = std::assume_aligned<CachelineSize>(
        reinterpret_cast<simd::Packed<value_type>*>(_persp_accum[side].data())
    );
    const auto* const _RESTRICT prev_side_acc = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(prev_accum->_persp_accum[side].data())
    );
    const auto* const _RESTRICT weights_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(weights.data())
    );

    assert(reinterpret_cast<ptrdiff_t>(side_acc) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(prev_side_acc) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    std::copy_n(prev_side_acc, PackedCount, side_acc);

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const size_t base_offset = removed_features[i] * PackedCount;

        accumulatorPolicyUpdate<PackedCount>(side_acc, weights_base + base_offset, 
            [](const auto& acc_val, const auto& weight) {
                return acc_val.aligned_load_this() - weight.aligned_load_this();
            });
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const size_t base_offset = added_features[i] * PackedCount;

        accumulatorPolicyUpdate<PackedCount>(side_acc, weights_base + base_offset, 
            [](const auto& acc_val, const auto& weight) {
                return acc_val.aligned_load_this() + weight.aligned_load_this();
            });
    }

#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _persp_accum[side][i] = prev_accum->_persp_accum[side][i];
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const size_t base_offset = removed_features[i] * NetworkAccumulatorSizePerSide;
        
        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _persp_accum[side][j] -= weights[base_offset + j];
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const size_t base_offset = added_features[i] * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _persp_accum[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::clear(enumColor side) {
    _persp_accum[side].fill(0);
}

std::span<const Accumulator::value_type, NetworkAccumulatorSizePerSide> 
Accumulator::getValues(enumColor side) const {
    return std::span<const value_type, NetworkAccumulatorSizePerSide>(_persp_accum[side].data(), NetworkAccumulatorSizePerSide);
}

#if defined(_VERIFY_NN)
bool Accumulator::verify(const Accumulator& accum, const Position& pos) {
    Accumulator ref_accum(nn::GlobPackedNetwork, pos);
    return accum == ref_accum;
}
#endif

template uint16_t Accumulator::featureIndex<WHITE>(Square sq, Piece::enumType piece_type, enumColor side);
template uint16_t Accumulator::featureIndex<BLACK>(Square sq, Piece::enumType piece_type, enumColor side);

} // namespace nn
