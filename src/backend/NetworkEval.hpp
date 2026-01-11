#pragma once

#include "PackedNetwork.hpp"
#include "Score.hpp"
#include "Accumulator.hpp"

struct NodeInfo;

namespace nn {

class NEval {
public:
    // evaluates a position without accumulators.
    static Score evaluate(const PackedNeuralNetwork& network, std::string fen);
    static Score evaluate(const PackedNeuralNetwork& network, const Position& pos);

    // evalutes a positions using computed accumulators
    static Score evaluate(const PackedNeuralNetwork& network, const Accumulator& acc, enumColor side2move);
private:
    static int32_t layerActivationSingleOutput(const int16_t* _RESTRICT s2m_accumulator, 
                                               const int16_t* _RESTRICT ns2m_accumulator,
                                               const PackedNeuralNetwork& network);
};

} // namespace nn
