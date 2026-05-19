#include "Accumulator.hpp"
#include "Memory.hpp"

namespace nn {

template <enumColor Perspective>
int Accumulator::featureIndex(Square sq, 
                              Piece::enumType piece_type, 
                              enumColor side) 
{
    if constexpr (Perspective == BLACK) {
        return static_cast<int>(!side) * 64 * 6 
            + value(piece_type) * 64 
            + static_cast<int>(verticalFlip(sq));
    }

    return static_cast<int>(side) * 64 * 6 
        + value(piece_type) * 64 
        + static_cast<int>(sq);
}

int Accumulator::featureIndex(enumColor perspective, 
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

    array2d<int, 2, 32> side_active_features;

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
                          const int* _RESTRICT side_active_features,
                          size_t side_active_features_cnt) 
{
    assert(biases != nullptr);
    assert(weights != nullptr);
    assert(side_active_features != nullptr);

#if defined(_NN_USE_AVX512) or defined(_NN_USE_AVX2) or defined(_NN_USE_SSE2) // Use SIMD Extensions

    static constexpr int RegisterWidth = MaxRegisterSizeBits / 16;
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    _max_platf_register_i_t* const _RESTRICT values_base = (_max_platf_register_i_t*)_values[side].data();
    const _max_platf_register_i_t* const _RESTRICT biases_base = (_max_platf_register_i_t*)biases;
    const _max_platf_register_i_t* const _RESTRICT weights_base = (_max_platf_register_i_t*)weights;

    assert(reinterpret_cast<size_t>(values_base) % AlignmentBound == 0);
    assert(reinterpret_cast<size_t>(weights_base) % AlignmentBound == 0);

    for (size_t i = 0; i < ChunkCount; i++) {
        _max_register_aligned_store_i(values_base + i, _max_register_aligned_load_i(biases_base + i));
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const int index = side_active_features[i];
        const int base_offset = index * ChunkCount;

        for (size_t j = 0; j < ChunkCount; j++) {
            _max_register_aligned_store_i(values_base + j, _max_register_add_i16(values_base[j], weights_base[base_offset + j]));
        }
    }
    
#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _values[side][i] = biases[i];
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const int index = side_active_features[i];
        const int base_offset = index * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::update(const PackedNeuralNetwork& network,
                         const Accumulator* _RESTRICT prev_acc,
                         const int* _RESTRICT added_features,
                         size_t added_features_cnt,
                         const int* _RESTRICT removed_features,
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
                         const int* _RESTRICT added_features,
                         size_t added_features_cnt,
                         const int* _RESTRICT removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    assert(weights != nullptr);
    assert(prev_acc != nullptr);
    assert(added_features != nullptr);
    assert(removed_features != nullptr);
    assert(this != prev_acc);

#if defined(_NN_USE_AVX512) or defined(_NN_USE_AVX2) or defined(_NN_USE_SSE2) // Use SIMD Extensions

    static constexpr int RegisterWidth = MaxRegisterSizeBits / 16;
    static constexpr int ChunkCount = NetworkAccumulatorSizePerSide / RegisterWidth;

    static_assert(NetworkAccumulatorSizePerSide % ChunkCount == 0);

    _max_platf_register_i_t* const _RESTRICT values_base = (_max_platf_register_i_t*)_values[side].data();
    const _max_platf_register_i_t* const _RESTRICT prev_values_base = (_max_platf_register_i_t*)prev_acc->_values[side].data();
    const _max_platf_register_i_t* const _RESTRICT weights_base = (_max_platf_register_i_t*)weights;

    assert(reinterpret_cast<size_t>(values_base) % AlignmentBound == 0);
    assert(reinterpret_cast<size_t>(prev_values_base) % AlignmentBound == 0);
    assert(reinterpret_cast<size_t>(weights_base) % AlignmentBound == 0);

    for (size_t i = 0; i < ChunkCount; i++) {
        _max_register_aligned_store_i(values_base + i, _max_register_aligned_load_i(prev_values_base + i));
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const int index = removed_features[i];
        const int base_offset = index * ChunkCount;
        
        for (size_t j = 0; j < ChunkCount; j++) {
            _max_register_aligned_store_i(values_base + j, _max_register_sub_i16(values_base[j], weights_base[base_offset + j]));
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const int index = added_features[i];
        const int base_offset = index * ChunkCount;

        for (size_t j = 0; j < ChunkCount; j++) {
            _max_register_aligned_store_i(values_base + j, _max_register_add_i16(values_base[j], weights_base[base_offset + j]));
        }
    }

#else // Do not use SIMD Extensions

    for (size_t i = 0; i < NetworkAccumulatorSizePerSide; i++) {
        _values[side][i] = prev_acc->_values[side][i];
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const int index = removed_features[i];
        const int base_offset = index * NetworkAccumulatorSizePerSide;
        
        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] -= weights[base_offset + j];
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const int index = added_features[i];
        const int base_offset = index * NetworkAccumulatorSizePerSide;

        for (size_t j = 0; j < NetworkAccumulatorSizePerSide; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }

#endif // Do not use SIMD Extensions
}

void Accumulator::clear(enumColor side) {
    alignedMemset(_values[side].data(), 0, NetworkAccumulatorSizePerSide);
}

const array1d<int16_t, NetworkHiddenLayerSize>& Accumulator::getValues(enumColor side) const {
    return _values[side];
}

#if defined(_VERIFY_NN)
bool Accumulator::verify(const Accumulator& accum, const Position& pos) {
    Accumulator ref_accum(nn::GlobPackedNetwork, pos);
    return accum == ref_accum;
}
#endif

void AccumulatorCache::clearBuffers() {
    added_features_cnt = 0;
    removed_features_cnt = 0;
    dirty = false;
}

template int Accumulator::featureIndex<WHITE>(Square sq, Piece::enumType piece_type, enumColor side);
template int Accumulator::featureIndex<BLACK>(Square sq, Piece::enumType piece_type, enumColor side);

} // namespace nn
