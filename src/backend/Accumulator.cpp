#include "Accumulator.hpp"

namespace nn {

bool Accumulator::operator==(const Accumulator& accum) const {
    return !std::memcmp(_values, accum._values, sizeof(_values));
}

template <enumColor Perspective>
int Accumulator::featureIndex(Square sq, Piece::enumType piece_type, enumColor side) 
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

void Accumulator::refresh(const int16_t* biases, 
                          const int16_t* weights, 
                          const Position& pos) 
{   
    assert(biases != nullptr);
    assert(weights != nullptr);

    int side_active_features[2][32];

    size_t active_features_cnt = 0;

    for (Piece::enumType piece_type : Piece::PieceTypeList) {
        for (enumColor side : { WHITE, BLACK }) {
            BitBoard bb = pos.get(piece_type, side);

            while (bb) {
                Square sq = static_cast<Square>(bb.dropForward());
                
                side_active_features[WHITE][active_features_cnt] = featureIndex<WHITE>(sq, piece_type, side);
                side_active_features[BLACK][active_features_cnt] = featureIndex<BLACK>(sq, piece_type, side);

                ++active_features_cnt;
                assert(active_features_cnt <= 32);
            }
        }
    }

    for (enumColor persp : { WHITE, BLACK })
        refresh(biases, weights, persp, side_active_features[persp], active_features_cnt);
}

void Accumulator::refresh(const int16_t* biases, 
                          const int16_t* weights, 
                          enumColor side, 
                          int* side_active_features,
                          size_t side_active_features_cnt) 
{
    assert(biases != nullptr);
    assert(weights != nullptr);
    assert(side_active_features != nullptr);

    for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
        _values[side][i] = biases[i];
    }

    for (size_t i = 0; i < side_active_features_cnt; i++) {
        const int index = side_active_features[i];
        const int base_offset = index * NetworkAccumulatorSize;

        for (size_t j = 0; j < NetworkAccumulatorSize; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }
}

void Accumulator::update(const int16_t* weights,
                         const Accumulator* prev_acc,
                         int* added_features,
                         size_t added_features_cnt,
                         int* removed_features,
                         size_t removed_features_cnt,
                         enumColor side)
{
    assert(weights != nullptr);
    assert(prev_acc != nullptr);
    assert(added_features != nullptr);
    assert(removed_features != nullptr);
    assert(this != prev_acc);

    for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
        _values[side][i] = prev_acc->_values[side][i];
    }

    for (size_t i = 0; i < removed_features_cnt; i++) {
        const int index = removed_features[i];
        const int base_offset = index * NetworkAccumulatorSize;
        
        for (size_t j = 0; j < NetworkAccumulatorSize; j++) {
            _values[side][j] -= weights[base_offset + j];
        }
    }

    for (size_t i = 0; i < added_features_cnt; i++) {
        const int index = added_features[i];
        const int base_offset = index * NetworkAccumulatorSize;

        for (size_t j = 0; j < NetworkAccumulatorSize; j++) {
            _values[side][j] += weights[base_offset + j];
        }
    }
}

void Accumulator::clear(enumColor side) {
    alignedMemset(_values[side], 0, NetworkAccumulatorSize);
}

const int16_t* Accumulator::getValues(enumColor side) const {
    return _values[side];
}

#if defined(_VERIFY_NN)
bool Accumulator::verify(const Accumulator& accum, const Position& pos) {
    Accumulator ref_accum;
    ref_accum.refresh(nn::GlobPackedNetwork.getLayerBiases(0),
                      nn::GlobPackedNetwork.getLayerWeights(0),
                      pos);
    return accum == ref_accum;
}
#endif

template int Accumulator::featureIndex<WHITE>(Square sq, Piece::enumType piece_type, enumColor side);
template int Accumulator::featureIndex<BLACK>(Square sq, Piece::enumType piece_type, enumColor side);

} // namespace nn
