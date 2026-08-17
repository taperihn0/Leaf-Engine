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

#include "Cuckoo.hpp"
#include "Memory.hpp"
#include "BitBoard.hpp"
#include "Attacks.hpp"
#include "Hash.hpp"

CuckooTables::CuckooTables()
    : _cuckoo_entry_buff(
        mem::makeAlignedUnique<CuckooEntry>(_CuckooTableSize, CachelineSize)) { 
    mem::memSet(_cuckoo_entry_buff.get(), 0, sizeof(CuckooEntry) * _CuckooTableSize);
}

void CuckooTables::init() {
    for (auto side : { WHITE, BLACK }) {
        for (auto piece : Piece::PieceTypeList) {
            if (piece == Piece::PAWN or piece == Piece::QUEEN) 
                continue;

            for (uint8_t from = 0; from < 64; from++) {
                for (uint8_t to = from + 1; to < 64; to++) {
                    BitBoard attacks = BitBoard::Empty;

                    switch (piece) {
                    case Piece::KNIGHT:
                        attacks = knightAttacks(from);
                        break;
                    case Piece::BISHOP:
                        attacks = rayAttacksBishop(from);
                        break;
                    case Piece::ROOK:
                        attacks = rayAttacksRook(from);
                        break;
                    case Piece::KING:
                        attacks = kingAttacks(from);
                        break;
                    default:
                        break;
                    }

                    if (!attacks.isOccupiedSq(to)) 
                        continue;

                    uint32_t move_hash = static_cast<uint32_t>(ZHashMasks->piece_keys[side][piece][from] ^ 
                                                               ZHashMasks->piece_keys[side][piece][to] ^ 
                                                               ZHashMasks->black_key);

                    Move16b move16b = Move16b::makePackedSimple(from, to);

                    size_t idx = cuckooIndex1(move_hash);

                    for (uint kick = 0; kick < _KickThreshold; kick++) {
                        std::swap(_cuckoo_entry_buff.get()[idx].move_hash, move_hash);
                        std::swap(_cuckoo_entry_buff.get()[idx].move16, move16b);

                        ASSERT((!move_hash and move16b.isNullMove()) or (move_hash and !move16b.isNullMove()), 
                                "Invalid entry in cuckoo tables");

                        if (move16b.isNullMove())
                            break;

                        idx = idx == cuckooIndex1(move_hash) ? cuckooIndex2(move_hash) 
                                                             : cuckooIndex1(move_hash);
                    }
                }
            }
        }
    }

    validate();
}

void CuckooTables::validate() {
    size_t count = 0;

    for (size_t i = 0; i < _CuckooTableSize; i++) {
        const uint32_t move_hash = _cuckoo_entry_buff.get()[i].move_hash;
        const Move16b move16b = _cuckoo_entry_buff.get()[i].move16;

        ASSERT((!move_hash and move16b.isNullMove()) or (move_hash and !move16b.isNullMove()),
               "Invalid entry in cuckoo tables");

        if (move_hash and !move16b.isNullMove())
            count++;
    }

#ifdef DEBUG
    const float fill_rate = static_cast<float>(count) / _AccurateCount;
    std::cout << "[CUCKOO TABLES STATS]\n total entries: " << count 
              << "\n fill rate: " << fill_rate * 100 << '%' << std::endl;
#endif
}
