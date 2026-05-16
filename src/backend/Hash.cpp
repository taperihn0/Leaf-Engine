#include "Hash.hpp"
#include "Position.hpp"

#include <random>

_INLINE uint64_t sparseRandom() {
	return random<uint64_t>(1, maxof<uint64_t>())
		 & random<uint64_t>(1, maxof<uint64_t>());
}

ZobristMasks& ZobristMasks::get() {
	static ZobristMasks ZKeys;
	return ZKeys;
}

ZobristMasks::ZobristMasks() {
	fillKeys();
}

void ZobristMasks::fillKeys() {
	for (int sq = 0; sq < 64; sq++) {
		for (enumColor col : { WHITE, BLACK }) {
			for (auto piece_t : Piece::PieceTypeList) {
				piece_keys[col][piece_t][sq] = sparseRandom();
			}
		}
	}

	black_key = sparseRandom();

	for (int file = 0; file < 8; file++) {
		ep_file_keys[file] = sparseRandom();
	}

	for (enumColor col : { WHITE, BLACK }) {
		short_castle_keys[col] = sparseRandom();
		long_castle_keys[col] = sparseRandom();
	}
}

ZHash ZHash::generateOnFly(const Position& pos) {
	uint64_t key = 0;

	for (int sq = 0; sq < 64; sq++) {
		const Piece piece = pos.fullPieceOn(sq);

		if (piece.type() != Piece::NONE)
			key ^= ZHashMasks->piece_keys[piece.color()][piece.type()][sq];
	}

	if (pos.getTurn() == BLACK)
		key ^= ZHashMasks->black_key;

	const Square ep_sq = pos.getEnPassantSq();

	assert(ep_sq.isValid());
	if (!ep_sq.isNull())
		key ^= ZHashMasks->ep_file_keys[ep_sq.getFile()];

	if (pos.getCastlingByColor(WHITE).isShortPossible())
		key ^= ZHashMasks->short_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isShortPossible())
		key ^= ZHashMasks->short_castle_keys[BLACK];

	if (pos.getCastlingByColor(WHITE).isLongPossible())
		key ^= ZHashMasks->long_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isLongPossible())
		key ^= ZHashMasks->long_castle_keys[BLACK];

	return static_cast<ZHash>(key);
}

#if defined(_DEBUG)
bool ZobristHash::printXOR_Diff(uint64_t key_2) {
	std::cout << (_key ^ key_2) << ' ';
	return true;
}
#endif
