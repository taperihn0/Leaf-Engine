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

#include "NetworkEval.hpp"
#include "Search.hpp"

namespace nn {

_FORCEINLINE int16_t crelu(int16_t value, int16_t mi, int16_t ma) {
    return std::clamp(value, mi, ma);
}

_FORCEINLINE int32_t screlu(int16_t value, int16_t mi, int16_t ma) {
    const int32_t c = static_cast<int32_t>(crelu(value, mi, ma));
    return c * c;
}

sc::Score NEval::evaluate(const PackedNeuralNetwork& network, std::string fen) {
    return evaluate(network, Position(fen));
}

sc::Score NEval::evaluate(const PackedNeuralNetwork& network, const Position& pos) {
    assert(network.isValid());

    Accumulator accumulator;
    accumulator.refresh(network.getLayerBiases(0), network.getLayerWeights(0), pos);
    
    return evaluate(network, accumulator, pos.getTurn());
}

sc::Score NEval::evaluate(const PackedNeuralNetwork& network, 
                          const Accumulator& acc, 
                          enumColor side2move) 
{
    assert(network.isValid());

    const int16_t output = layerActivationSingleOutput(acc.getValues(side2move),
                                                       acc.getValues(!side2move),
                                                       network);
    return static_cast<sc::Score>(output);
}

int32_t NEval::layerActivationSingleOutput(std::span<const int16_t, NetworkAccumulatorSizePerSide> s2m_accumulator, 
                                           std::span<const int16_t, NetworkAccumulatorSizePerSide> ns2m_accumulator,
                                           const PackedNeuralNetwork& network) 
{
    // TODO: use dedicated value_type instead of int16_t. If not present - define it.

    const auto weights = network.getLayerWeights(1);
    const auto output_bias = network.getLayerBiases(1)[0];

    int32_t output = 0;

#if defined(_NN_USE_SCRELU_SIMD)

    static constexpr int RegisterWidth = simd::getSimdRegisterSizeBits() / (sizeof(int16_t) * 8);
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    auto sum_vec = simd::Packed<int16_t>::zero();
    const auto qa_vec = simd::Packed<int16_t>::fill(NetworkWeightQuant);
    const auto zero_vec = simd::Packed<int16_t>::zero();

    const auto const _RESTRICT s2m_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<int16_t>*>(s2m_accumulator.data())
    );
    const auto const _RESTRICT ns2m_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<int16_t>*>(ns2m_accumulator.data())
    );
    const auto const _RESTRICT weights_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<int16_t>*>(weights.data())
    );

    assert(reinterpret_cast<size_t>(s2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(ns2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    // TODO: that should be moved to PackedNetwork::isValid()
    // Assert that we won't overflow in int16 range.
#if defined(_NN_VERIFY_SCRELU_OVERFLOW)
    static constexpr int Int16Max = maxof<int16_t>();
    static constexpr int MaxWeight = Int16Max / NetworkWeightQuant;

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        ASSERT_NO_LOG(weights[i] >= -MaxWeight and weights[i] <= MaxWeight);
        ASSERT_NO_LOG(weights[NetworkAccumulatorSizePerSide + i] >= -MaxWeight 
                     and weights[NetworkAccumulatorSizePerSide + i] <= MaxWeight);

        const int32_t mul0 = static_cast<int32_t>(weights[i]) 
                             * static_cast<int32_t>(NetworkWeightQuant);
        ASSERT_NO_LOG(mul0 >= -Int16Max and mul0 <= Int16Max);

        const int32_t mul1 = static_cast<int32_t>(weights[NetworkAccumulatorSizePerSide + i]) 
                             * static_cast<int32_t>(NetworkWeightQuant);
        ASSERT_NO_LOG(mul1 >= -Int16Max and mul1 <= Int16Max);
    }
#endif

    // TODO: should have written another method abstracting iteration through one-sided accum,
    // then call it twice here

    for (size_t i = 0; i < ChunkCount; i++) {
        const auto  s2m_clamp = simd::Packed<int16_t>::min(
            simd::Packed<int16_t>::max(s2m_base[i], zero_vec), qa_vec
        );

        const auto rs2m  = (weights_base[i] * s2m_clamp).madd(s2m_clamp);
        sum_vec += rs2m;
    }

    for (size_t i = 0; i < ChunkCount; i++) {
        const auto ns2m_clamp = simd::Packed<int16_t>::min(
            simd::Packed<int16_t>::max(ns2m_base[i], zero_vec), qa_vec
        );

        const auto rns2m = (weights_base[i + ChunkCount] * ns2m_clamp).madd(ns2m_clamp);
        sum_vec += rns2m;
    }

    output = sum_vec.accumulate();

#else

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        output += screlu(s2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[i];
        output += screlu(ns2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[NetworkAccumulatorSizePerSide + i];
    }

#endif
    
    static constexpr auto DivFactor = NetworkWeightQuant * NetworkBiasQuant;
    output = (output / NetworkWeightQuant + output_bias) * NetworkOutputScale / DivFactor;

    return output;
}

} // namespace nn
