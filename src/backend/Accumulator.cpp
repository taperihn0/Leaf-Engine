#include "Accumulator.hpp"

namespace nn {

int Accumulator::activeFeatureIndex(enumColor perspective, Square sq, Piece piece) {
    if (perspective == BLACK) {
        return static_cast<int>(!piece.color()) * 64 * 6 
            + value(piece.type()) * 64 
            + static_cast<int>(verticalFlip(sq));
    }

    return static_cast<int>(piece.color()) * 64 * 6 
        + value(piece.type()) * 64 
        + static_cast<int>(sq);
}

int Accumulator::activeFeatureIndex(enumColor perspective, Square sq, Piece::enumType piece_type, enumColor side) {
    return activeFeatureIndex(perspective, sq, Piece(side, piece_type));
}

void Accumulator::refresh(const int16_t* biases, 
                          const int16_t* weights, 
                          const Position& pos) 
{   
    int side_active_features[2][32];

    size_t active_features_cnt = 0;

    for (Piece::enumType piece_type : Piece::PieceTypeList) {
        for (enumColor side : { WHITE, BLACK }) {
            BitBoard bb = pos.get(piece_type, side);

            while (bb) {
                Square sq = static_cast<Square>(bb.dropForward());
                
                for (enumColor persp : { WHITE, BLACK })
                    side_active_features[persp][active_features_cnt] = activeFeatureIndex(persp, sq, piece_type, side);

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
    for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
        _values[side][i] = biases[i];
    }

    for (size_t k = 0; k < side_active_features_cnt; k++) {
        int index = side_active_features[k];

        for (size_t i = 0; i < NetworkAccumulatorSize; i++) {
            _values[side][i] += weights[index * NetworkAccumulatorSize + i];
        }
    }
}

const int16_t* Accumulator::getValues(enumColor side) const {
    return _values[side];
}

} // namespace nn
