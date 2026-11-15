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

using value_type = PackedNeuralNetwork::value_type;

_FORCEINLINE value_type crelu(value_type value, value_type mi, value_type ma) {
    return std::clamp(value, mi, ma);
}

_FORCEINLINE int32_t screlu(value_type value, value_type mi, value_type ma) {
    const int32_t c = static_cast<int32_t>(crelu(value, mi, ma));
    return c * c;
}

sc::Score NNInference::evaluate(const PackedNeuralNetwork& network, std::string fen) {
    return evaluate(network, Position(fen));
}

sc::Score NNInference::evaluate(const PackedNeuralNetwork& network, const Position& pos) {
    assert(network.isValid());

    Accumulator accumulator;
    accumulator.refresh(network.getLayerBiases(0), network.getLayerWeights(0), pos);
    
    return evaluate(network, accumulator, pos.getTurn());
}

sc::Score NNInference::evaluate(const PackedNeuralNetwork& network, 
                                const Accumulator& acc, 
                                enumColor side2move) 
{
    assert(network.isValid());

    const auto output = layerActivationSingleOutput(acc.getValues(side2move),
                                                    acc.getValues(!side2move),
                                                    network);
    return static_cast<sc::Score>(output);
}

#if defined(_NN_USE_SCRELU_SIMD)

template <size_t PackedCount>
void NNInference::halfLayerActivation(const simd::Packed<value_type>* const _RESTRICT accum,
                                      const simd::Packed<value_type>* const _RESTRICT weights,
                                      simd::Packed<int32_t>& store)
{
    static const auto zero_pack = simd::Packed<value_type>::zero();
    static const auto qa_pack = simd::Packed<value_type>::fill(NetworkWeightQuant);

    for (size_t i = 0; i < PackedCount; i++) {
        const auto v_clamp = simd::Packed<value_type>::min(
            simd::Packed<value_type>::max(
                simd::Packed<value_type>::aligned_load(accum + i), zero_pack), qa_pack
        );

        store += (simd::Packed<value_type>::aligned_load(weights + i) * v_clamp).madd(v_clamp);
    }
}

#endif // _NN_USE_SCRELU_SIMD

int32_t NNInference::layerActivationSingleOutput(std::span<const value_type, NetworkAccumulatorSizePerSide> s2m_accumulator, 
                                                 std::span<const value_type, NetworkAccumulatorSizePerSide> ns2m_accumulator,
                                                 const PackedNeuralNetwork& network) 
{
    const auto weights = network.getLayerWeights(1);
    const auto output_bias = network.getLayerBiases(1)[0];

    int32_t output = 0;

#if defined(_NN_USE_SCRELU_SIMD)

    const auto* const _RESTRICT s2m_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(s2m_accumulator.data())
    );
    const auto* const _RESTRICT ns2m_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(ns2m_accumulator.data())
    );
    const auto* const _RESTRICT weights_base = std::assume_aligned<CachelineSize>(
        reinterpret_cast<const simd::Packed<value_type>*>(weights.data())
    );

    assert(reinterpret_cast<ptrdiff_t>(s2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(ns2m_base) % simd::getSimdRegisterAlignment() == 0);
    assert(reinterpret_cast<ptrdiff_t>(weights_base) % simd::getSimdRegisterAlignment() == 0);

    static constexpr size_t PackedCount = NetworkAccumulatorSizePerSide / simd::Packed<value_type>::count();
    static_assert(NetworkAccumulatorSizePerSide % PackedCount == 0);

    auto sum_vec = simd::Packed<int32_t>::zero();

    halfLayerActivation<PackedCount>(s2m_base, weights_base, sum_vec);
    halfLayerActivation<PackedCount>(ns2m_base, weights_base + PackedCount, sum_vec);

    output = sum_vec.accumulate();

#else

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        output += screlu(s2m_accumulator[i], 0, NetworkWeightQuant) * weights[i];
        output += screlu(ns2m_accumulator[i], 0, NetworkWeightQuant) * weights[NetworkAccumulatorSizePerSide + i];
    }

#endif
    
    static constexpr auto DivFactor = NetworkWeightQuant * NetworkBiasQuant;
    output = (output / NetworkWeightQuant + output_bias) * NetworkOutputScale / DivFactor;

    return output;
}

} // namespace nn
