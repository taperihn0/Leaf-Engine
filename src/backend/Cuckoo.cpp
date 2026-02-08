#include "Cuckoo.hpp"
#include "Memory.hpp"
#include "BitBoard.hpp"
#include "Attacks.hpp"
#include "Hash.hpp"

CuckooTables::CuckooTables() {
	_cuckoo_move_hash_buff = reinterpret_cast<uint64_t*>(alignedMalloc(8 * _CuckooTableSize, 8));
	_cuckoo_move16_buff    = reinterpret_cast<Move16b*>(alignedMalloc(2 * _CuckooTableSize, 2));

	alignedMemset(_cuckoo_move_hash_buff, 0, 8 * _CuckooTableSize);
	alignedMemset(_cuckoo_move16_buff, 0, 2 * _CuckooTableSize);
}

void CuckooTables::init() {
	for (auto side : { WHITE, BLACK }) {
		for (auto piece : Piece::PieceTypeList) {
			if (piece == Piece::PAWN or piece == Piece::QUEEN) 
				continue;

			for (uint8_t from = 0; from < 64; from++) {
				for (uint8_t to = from + 1; to < 64; to++) {
					BitBoard attacks = 0_ui64;

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
					}

					if (attacks.isOccupiedSq(to)) {
						uint64_t move_hash = ZobristHash::piece_keys[side][piece][from] ^ 
											  ZobristHash::piece_keys[side][piece][to] ^ 
											  ZobristHash::black_key;

						Move16b move16b = makePackedSimple(from, to);

						size_t idx = cuckooIndex1(move_hash);

						for (uint kick = 0; kick < _KickThreshold; kick++) {
							std::swap(_cuckoo_move_hash_buff[idx], move_hash);
							std::swap(_cuckoo_move16_buff[idx], move16b);

							ASSERT((!move_hash and move16b.isNull()) or (move_hash and !move16b.isNull()), 
								   "Invalid entry in cuckoo tables");

							if (move16b.isNull())
								break;

							idx = idx == cuckooIndex1(move_hash) ? cuckooIndex2(move_hash) 
																   : cuckooIndex1(move_hash);
						}
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
		const uint64_t move_hash = _cuckoo_move_hash_buff[i];
		const Move16b move16b = _cuckoo_move16_buff[i];

		ASSERT((!move_hash and move16b.isNull()) or (move_hash and !move16b.isNull()),
			   "Invalid entry in cuckoo tables");

		if (move_hash and !move16b.isNull()) {
			count++;
		}
	}

#ifdef _DEBUG
	const float fill_rate = static_cast<float>(count) / _AccurateCount;
	std::cout << "[CUCKOO TABLES STATS]\n total entries: " << count 
			  << "\n fill rate: " << fill_rate * 100 << '%' << std::endl;
#endif
}
