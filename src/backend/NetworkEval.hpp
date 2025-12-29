#pragma once

#include "PackedNetwork.hpp"
#include "Score.hpp"
#include "Accumulator.hpp"

struct NodeInfo;

namespace nn {

class NEval {
public:
    static const Accumulator* getPrevAccum(const NodeInfo* node, const NodeInfo* root);

    // evaluates a position without accumulators.
    static Score evaluate(const PackedNeuralNetwork& network, std::string fen);
    static Score evaluate(const PackedNeuralNetwork& network, const Position& pos);

    // evalutes a positions using computed accumulators
    static Score evaluate(const PackedNeuralNetwork& network, const Accumulator& acc, enumColor side2move);
private:
    static int32_t layerActivationOutput(const int16_t* s2m_accumulator, 
                                         const int16_t* ns2m_accumulator,
                                         const PackedNeuralNetwork& network);
};

} // namespace nn
