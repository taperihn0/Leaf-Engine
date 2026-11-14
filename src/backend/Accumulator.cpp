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

void Accumulator::refresh(const int16_t* _RESTRICT biases, 
                          const int16_t* _RESTRICT weights, 
                          const Position& pos) 
{   
    assert(biases != nullptr);
    assert(weights != nullptr);

    MultiArray<uint16_t, 2, 32> side_active_features;

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
        refresh(biases, weights, persp, side_active_features[persp].data(), active_features_cnt);
    }
}

void Accumulator::refresh(const int16_t* _RESTRICT biases, 
                          const int16_t* _RESTRICT weights, 
                          enumColor side, 
                          const uint16_t* _RESTRICT side_active_features,
                          size_t side_active_features_cnt) 
{
    assert(biases != nullptr);
    assert(weights != nullptr);
    assert(side_active_features != nullptr);

#if defined(_NN_USE_AVX512) or defined(_NN_USE_AVX2) or defined(_NN_USE_SSE2) // Use SIMD Extensions

    using Intr = simd::PlatformPackedIntrinsics<int16_t>;
    using Reg = Intr::packed_type;

    static constexpr int RegisterWidth = simd::getSimdRegisterSizeBits() / (sizeof(int16_t) * 8);
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    Reg* const _RESTRICT values_base = reinterpret_cast<Reg*>(_values[side].data());
    const Reg* const _RESTRICT biases_base = reinterpret_cast<const Reg*>(biases);
    const Reg* const _RESTRICT weights_base = reinterpret_cast<const Reg*>(weights);

    assert(reinterpret_cast<size_t>(values_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(biases_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    for (size_t i = 0; i < ChunkCount; i++) {
        Intr::store(values_base + i, Intr::load(biases_base + i));
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const size_t base_offset = side_active_features[i] * ChunkCount;

        for (size_t j = 0; j < ChunkCount; j++) {
            Intr::store(values_base + j, Intr::add(values_base[j], weights_base[base_offset + j]));
        }
    }
    
#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _values[side][i] = biases[i];
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const size_t base_offset = side_active_features[i] * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::update(const PackedNeuralNetwork& network,
                         const Accumulator* _RESTRICT prev_acc,
                         const uint16_t* _RESTRICT added_features,
                         size_t added_features_cnt,
                         const uint16_t* _RESTRICT removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    update(network.getLayerWeights(0),
           prev_acc, 
           added_features,
           added_features_cnt,
           removed_features,
           removed_features_cnt,
           side);
}


void Accumulator::update(const int16_t* _RESTRICT weights,
                         const Accumulator* _RESTRICT prev_acc,
                         const uint16_t* _RESTRICT added_features,
                         size_t added_features_cnt,
                         const uint16_t* _RESTRICT removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    assert(weights != nullptr);
    assert(prev_acc != nullptr);
    assert(added_features != nullptr);
    assert(removed_features != nullptr);
    assert(this != prev_acc);

#if defined(_NN_USE_AVX512) or defined(_NN_USE_AVX2) or defined(_NN_USE_SSE2) // Use SIMD Extensions

    using Intr = simd::PlatformPackedIntrinsics<int16_t>;
    using Reg = Intr::packed_type;

    static constexpr int RegisterWidth = simd::getSimdRegisterSizeBits() / (sizeof(int16_t) * 8);
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    Reg* const _RESTRICT values_base = reinterpret_cast<Reg*>(_values[side].data());
    const Reg* const _RESTRICT prev_values_base = reinterpret_cast<const Reg*>(prev_acc->_values[side].data());
    const Reg* const _RESTRICT weights_base = reinterpret_cast<const Reg*>(weights);

    assert(reinterpret_cast<size_t>(values_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(prev_values_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    for (size_t i = 0; i < ChunkCount; i++) {
        Intr::store(values_base + i, Intr::load(prev_values_base + i));
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const size_t base_offset = removed_features[i] * ChunkCount;
        
        for (size_t j = 0; j < ChunkCount; j++) {
            Intr::store(values_base + j, Intr::sub(values_base[j], weights_base[base_offset + j]));
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const size_t base_offset = added_features[i] * ChunkCount;

        for (size_t j = 0; j < ChunkCount; j++) {
            Intr::store(values_base + j, Intr::add(values_base[j], weights_base[base_offset + j]));
        }
    }

#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _values[side][i] = prev_acc->_values[side][i];
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const size_t base_offset = removed_features[i] * NetworkAccumulatorSizePerSide;
        
        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] -= weights[base_offset + j];
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const size_t base_offset = added_features[i] * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::clear(enumColor side) {
    mem::memSet(_values[side].data(), 0, NetworkAccumulatorSizePerSide);
}

std::span<const Accumulator::value_type, NetworkAccumulatorSizePerSide> 
Accumulator::getValues(enumColor side) const {
    return std::span<const value_type, NetworkAccumulatorSizePerSide>(_values[side].data());
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
