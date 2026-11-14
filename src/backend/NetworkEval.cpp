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

    const int16_t output = layerActivationSingleOutput(acc.getValues(side2move).data(),
                                                       acc.getValues(!side2move).data(),
                                                       network);
    return static_cast<sc::Score>(output);
}

int32_t NEval::layerActivationSingleOutput(const int16_t* _RESTRICT s2m_accumulator, 
                                           const int16_t* _RESTRICT ns2m_accumulator,
                                           const PackedNeuralNetwork& network) 
{
    const int16_t* weights = network.getLayerWeights(1);
    int32_t output_bias = *network.getLayerBiases(1);

    int32_t output = 0;

#if defined(_NN_USE_SCRELU_SIMD)

    using Intr = simd::PlatformPackedIntrinsics<int16_t>;
    using Reg = Intr::packed_type;

    static constexpr int RegisterWidth = simd::getSimdRegisterSizeBits() / (sizeof(int16_t) * 8);
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    Reg sum_vec = Intr::zero();
    const Reg qa_vec = Intr::fill(NetworkWeightQuant);
    const Reg zero_vec = Intr::zero();

    const Reg* const _RESTRICT s2m_base = reinterpret_cast<const Reg*>(s2m_accumulator);
    const Reg* const _RESTRICT ns2m_base = reinterpret_cast<const Reg*>(ns2m_accumulator);
    const Reg* const weights_base = reinterpret_cast<const Reg*>(weights);

    assert(reinterpret_cast<size_t>(s2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(ns2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<size_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

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

    for (size_t i = 0; i < ChunkCount; i++) {
        const Reg  s2m_clamp = Intr::min(Intr::max(s2m_base[i], zero_vec), qa_vec);
        const Reg ns2m_clamp = Intr::min(Intr::max(ns2m_base[i], zero_vec), qa_vec);

        const Reg rs2m  = Intr::madd(Intr::mul(weights_base[i], s2m_clamp ), s2m_clamp);
        const Reg rns2m = Intr::madd(Intr::mul(weights_base[i + ChunkCount], ns2m_clamp), ns2m_clamp);

        sum_vec = simd::PlatformPackedIntrinsics<int32_t>::add(sum_vec, rs2m);
        sum_vec = simd::PlatformPackedIntrinsics<int32_t>::add(sum_vec, rns2m);
    }

    output = Intr::accumulate(sum_vec);

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
