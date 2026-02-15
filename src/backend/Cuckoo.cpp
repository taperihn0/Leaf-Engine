#include "Cuckoo.hpp"
#include "Memory.hpp"
#include "BitBoard.hpp"
#include "Attacks.hpp"
#include "Hash.hpp"

CuckooTables::CuckooTables() {
	_cuckoo_entry_buff = new _CuckooEntry[_CuckooTableSize];
	memset(_cuckoo_entry_buff, 0, sizeof(_CuckooEntry) * _CuckooTableSize);
}

CuckooTables::~CuckooTables() {
	delete[] _cuckoo_entry_buff;
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
					default:
						break;
					}

					if (attacks.isOccupiedSq(to)) {
						uint32_t move_hash = static_cast<uint32_t>(ZobristHash::piece_keys[side][piece][from] ^ 
																	ZobristHash::piece_keys[side][piece][to] ^ 
																	ZobristHash::black_key);

						Move16b move16b = makePackedSimple(from, to);

						size_t idx = cuckooIndex1(move_hash);

						for (uint kick = 0; kick < _KickThreshold; kick++) {
							std::swap(_cuckoo_entry_buff[idx].move_hash, move_hash);
							std::swap(_cuckoo_entry_buff[idx].move16, move16b);

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
		const uint32_t move_hash = _cuckoo_entry_buff[i].move_hash;
		const Move16b move16b = _cuckoo_entry_buff[i].move16;

		ASSERT((!move_hash and move16b.isNull()) or (move_hash and !move16b.isNull()),
			   "Invalid entry in cuckoo tables");

		if (move_hash and !move16b.isNull())
			count++;
	}

#ifdef _DEBUG
	const float fill_rate = static_cast<float>(count) / _AccurateCount;
	std::cout << "[CUCKOO TABLES STATS]\n total entries: " << count 
			  << "\n fill rate: " << fill_rate * 100 << '%' << std::endl;
#endif
}
