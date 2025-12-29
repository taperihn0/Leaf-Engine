#include "NetworkEval.hpp"
#include "Search.hpp"

namespace nn
{

const Accumulator* NEval::getPrevAccum(const NodeInfo* node, const NodeInfo* preroot) {
    for (const NodeInfo* hist_node = node - 1; hist_node != preroot; hist_node--) {
        if (hist_node->move != Move32b::Null)
            return &hist_node->accum;
    }

    return &preroot->accum;
}

INLINE int16_t crelu(int16_t value, int16_t mi, int16_t ma) {
    return std::clamp(value, mi, ma);
}

INLINE int32_t screlu(int16_t value, int16_t mi, int16_t ma) {
    const int32_t c = static_cast<int32_t>(crelu(value, mi, ma));
    return c * c;
}

Score NEval::evaluate(const PackedNeuralNetwork& network, std::string fen) {
    return evaluate(network, Position(fen));
}

Score NEval::evaluate(const PackedNeuralNetwork& network, const Position& pos) {
    ASSERTNOLOG(network.isValid());

    Accumulator accumulator;
    accumulator.refresh(network.getLayerBiases(0), network.getLayerWeights(0), pos);

    return evaluate(network, accumulator, pos.getTurn());
}

Score NEval::evaluate(const PackedNeuralNetwork& network, const Accumulator& acc, enumColor side2move) {
    ASSERTNOLOG(network.isValid());

    const int16_t output = layerActivationOutput(acc.getValues(side2move), 
                                                 acc.getValues(!side2move),
                                                 network);
    return static_cast<Score>(output);
}

int32_t NEval::layerActivationOutput(const int16_t* s2m_accumulator, 
                                             const int16_t* ns2m_accumulator,
                                             const PackedNeuralNetwork& network) 
{
    const int16_t* weights = network.getLayerWeights(1);
    int32_t output_bias = *network.getLayerBiases(1);

    int32_t output = 0;

    for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
        output += screlu(s2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[i];
        output += screlu(ns2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[NetworkAccumulatorSize + i];
    }

    output = (output / NetworkWeightQuant + output_bias) 
              * NetworkOutputScale / (NetworkWeightQuant * NetworkBiasQuant);
              
    return output;
}

} // namespace nn
