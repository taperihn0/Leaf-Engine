#pragma once

#include "PackedNetwork.hpp"
#include "Score.hpp"
#include "Accumulator.hpp"

namespace nn {

class NeuralNetwork {
public:
    // evaluates a position without accumulators.
    static Score evaluate(const PackedNeuralNetwork& network, std::string fen);
    static Score evaluate(const PackedNeuralNetwork& network, const Position& pos);
private:
    static int32_t layerActivationOutput(const int16_t* s2m_accumulator, 
                                         const int16_t* ns2m_accumulator,
                                         const PackedNeuralNetwork& network);
};

} // namespace nn
