#include "NetworkEval.hpp"
#include "Search.hpp"

namespace nn {

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

    const int16_t output = layerActivationSingleOutput(acc.getValues(side2move), 
                                                       acc.getValues(!side2move),
                                                       network);
    return static_cast<Score>(output);
}

int32_t NEval::layerActivationSingleOutput(const int16_t* _RESTRICT s2m_accumulator, 
                                           const int16_t* _RESTRICT ns2m_accumulator,
                                           const PackedNeuralNetwork& network) 
{
    static constexpr int DivFactor = NetworkWeightQuant * NetworkBiasQuant;

    const int16_t* weights = network.getLayerWeights(1);
    int32_t output_bias = *network.getLayerBiases(1);

    int32_t output = 0;

#if defined(_NN_USE_SCRELU_SIMD)

    static constexpr int RegisterWidth = MaxRegisterSizeBits / 16;
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    _max_platf_register_i_t sum_vec = _max_register_zero_i;
    const _max_platf_register_i_t qa_vec = _max_register_fill_i16(NetworkWeightQuant);
    const _max_platf_register_i_t zero_vec = _max_register_zero_i;

    const _max_platf_register_i_t* const _RESTRICT s2m_base = (_max_platf_register_i_t*)s2m_accumulator;
    const _max_platf_register_i_t* const _RESTRICT ns2m_base = (_max_platf_register_i_t*)ns2m_accumulator;
    const _max_platf_register_i_t* const weights_base = (_max_platf_register_i_t*)weights;

    assert(reinterpret_cast<size_t>(s2m_base) % AlignmentBound == 0);
    assert(reinterpret_cast<size_t>(ns2m_base) % AlignmentBound == 0);
    assert(reinterpret_cast<size_t>(weights_base) % AlignmentBound == 0);

    // Assert that we won't overflow in int16 range.
#if defined(_NN_VERIFY_SCRELU_OVERFLOW)
    static constexpr int Int16Max = std::numeric_limits<int16_t>::max();
    static constexpr int MaxWeight = Int16Max / NetworkWeightQuant;

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        ASSERTNOLOG(weights[i] >= -MaxWeight and weights[i] <= MaxWeight);
        ASSERTNOLOG(weights[NetworkAccumulatorSizePerSide + i] >= -MaxWeight 
                    and weights[NetworkAccumulatorSizePerSide + i] <= MaxWeight);

        const int32_t mul0 = static_cast<int32_t>(weights[i]) 
                             * static_cast<int32_t>(NetworkWeightQuant);
        ASSERTNOLOG(mul0 >= -Int16Max and mul0 <= Int16Max);

        const int32_t mul1 = static_cast<int32_t>(weights[NetworkAccumulatorSizePerSide + i]) 
                             * static_cast<int32_t>(NetworkWeightQuant);
        ASSERTNOLOG(mul1 >= -Int16Max and mul1 <= Int16Max);
    }
#endif

    for (size_t i = 0; i < ChunkCount; i++) {
        const _max_platf_register_i_t  s2m_clamp = _max_register_min_i16(_max_register_max_i16(s2m_base[i], zero_vec), qa_vec);
        const _max_platf_register_i_t ns2m_clamp = _max_register_min_i16(_max_register_max_i16(ns2m_base[i], zero_vec), qa_vec);

        const __m256i rs2m  = _max_register_madd_i16(_max_register_mul_i16(weights_base[i], s2m_clamp ), s2m_clamp);
        const __m256i rns2m = _max_register_madd_i16(_max_register_mul_i16(weights_base[i + ChunkCount], ns2m_clamp), ns2m_clamp);

        sum_vec = _max_register_add_i32(sum_vec, rs2m);
        sum_vec = _max_register_add_i32(sum_vec, rns2m);
    }

    output = sumElements_i16(sum_vec);

#else

    for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
        output += screlu(s2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[i];
        output += screlu(ns2m_accumulator[i], 0, NetworkWeightQuant) 
                * weights[NetworkAccumulatorSize + i];
    }

#endif
    
    output = (output / NetworkWeightQuant + output_bias) * NetworkOutputScale / DivFactor;
    return output;
}

} // namespace nn
