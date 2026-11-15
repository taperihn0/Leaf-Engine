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
#include "Score.hpp"
#include "Accumulator.hpp"

struct NodeInfo;

namespace nn {

class NNInference {
public:
    using value_type = PackedNeuralNetwork::value_type;

    // evaluates a position without accumulators.
    _NODISCARD static sc::Score evaluate(const PackedNeuralNetwork& network, std::string fen);
    _NODISCARD static sc::Score evaluate(const PackedNeuralNetwork& network, const Position& pos);
    // evalutes a positions using computed accumulators
    _NODISCARD static sc::Score evaluate(const PackedNeuralNetwork& network, 
                                         const Accumulator& acc, 
                                         enumColor side2move);
private:
    _NODISCARD static int32_t layerActivationSingleOutput(std::span<const value_type, NetworkAccumulatorSizePerSide> s2m_accumulator, 
                                                          std::span<const value_type, NetworkAccumulatorSizePerSide> ns2m_accumulator,
                                                          const PackedNeuralNetwork& network);

    template <size_t PackedCount>
    static void halfLayerActivation(const simd::Packed<value_type>* const _RESTRICT accum,
                                    const simd::Packed<value_type>* const _RESTRICT weights,
                                    simd::Packed<value_type>& store);
};

} // namespace nn
