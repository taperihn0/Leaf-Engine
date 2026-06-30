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

#include "TrainEntry.hpp"

#include <sstream>

namespace Utils {

TrainingDataEntry::TrainingDataEntry(const PackedPosition& packed, Score white_score, Result8b result)
    : _packed_pos(packed)
    , _game_details{ white_score, result, packed.getKingSquare(), packed.getOppKingSquare() }
{}

TrainingDataEntry::TrainingDataEntry(const ExtPackedPosition& packed, Score white_score, Result8b result)
    : TrainingDataEntry(PackedPosition::fromExt(packed), white_score, result)
{}

bool TrainingDataEntry::write(std::ostream& output, const TrainingDataEntry& entry) {
    assert(output);

    if (!PackedPosition::writeStatic(output, entry._packed_pos)) {
        ASSERT(false, "Failed to write packed position of training entry to buffer");
        return false;
    }

    if (!output.write(reinterpret_cast<const char*>(&entry._game_details), sizeof(PackedPosInfo))) {
        ASSERT(false, "Failed to write training data entry to the output file");
        return false;
    }

    return output.good();
}

bool TrainingDataEntry::read(std::istream& input, TrainingDataEntry& entry) {
    assert(input);

    if (!PackedPosition::readStatic(input, entry._packed_pos))
        return false;

    if (!input.read(reinterpret_cast<char*>(&entry._game_details), sizeof(PackedPosInfo))) {
        ASSERT(false, "Failed to read training data game info from file");
        return false;
    }

    return input.good();
}

BulletChessBoard TrainingDataEntry::toBulletFormat(const TrainingDataEntry& entry) {
    Position pos = PackedPosition::unpacked(entry.getPosition());
    BitBoard rel_occ = pos.getOccupied();
    const enumColor side2move = pos.getTurn();

    if (side2move == BLACK) {
        rel_occ = rel_occ.swapBytes();
    }

    BulletChessBoard bullet_entry;

    // Occupancies
    bullet_entry.occ = rel_occ;

    // Nibbles
    bullet_entry.pcs.fill(0);

    BitBoard rel_own_pieces = pos.getOwnPieces();

    if (side2move == BLACK) {
        rel_own_pieces = rel_own_pieces.swapBytes();
    }

    for (size_t i = 0; rel_occ > 0; i++) {
        const Square sq(rel_occ.dropForward());
        const BitBoard bb(sq);

        uint8_t opp_piece = bb & rel_own_pieces ? 0 : 1;
        uint8_t val_piece = value(Piece::NONE);

        const Square abs_sq = side2move == BLACK ? sqVerticalFlip(sq) : sq;

        for (Piece::enumType pc : Piece::PieceTypeList) {
            const BitBoard pc_bb = pos.get(pc, opp_piece ? !side2move : side2move);

            if (pc_bb.isOccupiedSq(abs_sq)) {
                val_piece = value(pc);
                break;
            }
        }

        ASSERT(val_piece != value(Piece::NONE), "No piece found");

        const uint8_t mask = (opp_piece << 3) | val_piece;
        bullet_entry.pcs[i / 2] |= mask << (4 * (i & 1));
    }

    // Score
    const Score::int_t white_score = static_cast<Score::int_t>(entry.getWhiteScore());
    bullet_entry.score = side2move == WHITE ? white_score : -white_score;

    // Result
    const uint8_t result = static_cast<uint8_t>(entry.getGameResult());
    bullet_entry.result = side2move == WHITE ? result : 2 - result;

    // King squares
    if (side2move == WHITE) {
        bullet_entry.ksq = pos.getKingBySide(WHITE);
        bullet_entry.opp_ksq = sqVerticalFlip(pos.getKingSquareBySide(BLACK));
    } 
    else {
        bullet_entry.ksq = sqVerticalFlip(pos.getKingSquareBySide(BLACK));
        bullet_entry.opp_ksq = pos.getKingSquareBySide(BLACK);
    }

    return bullet_entry;
}

Score TrainingDataEntry::getWhiteScore() const {
    return _game_details.white_score;
}

TrainingDataEntry::Result8b TrainingDataEntry::getGameResult() const {
    return _game_details.result;
}

PackedPosition TrainingDataEntry::getPosition() const {
    return _packed_pos;
}

} // Utils
