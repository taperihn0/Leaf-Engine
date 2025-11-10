#include "Hash.hpp"
#include "Position.hpp"

#include <random>

static constexpr int RandomSeed = 0xfff;

INLINE uint64_t randomU64() {
	return random<uint64_t>(1, std::numeric_limits<uint64_t>::max(), 0xfff)
		   & random<uint64_t>(1, std::numeric_limits<uint64_t>::max(), 0xfff);
}

void ZobristHash::fillKeys() {
	for (int sq = 0; sq < 64; sq++) {
		for (enumColor col : { WHITE, BLACK }) {
			for (auto piece_t : Piece::piece_list) {
				piece_keys[col][piece_t][sq] = randomU64();
			}
		}
	}

	black_key = randomU64();

	for (int file = 0; file < 8; file++) {
		ep_file_keys[file] = randomU64();
	}

	for (enumColor col : { WHITE, BLACK }) {
		short_castle_keys[col] = randomU64();
		long_castle_keys[col] = randomU64();
	}
}

ZobristHash ZobristHash::generateOnFly(const Position& pos) {
	uint64_t key = 0;

	for (int sq = 0; sq < 64; sq++) {
		const Piece piece = pos.fullPieceOn(sq);

		if (piece.type() != Piece::NONE)
			key ^= piece_keys[piece.color()][piece.type()][sq];
	}

	if (pos.getTurn() == BLACK)
		key ^= black_key;

	const Square ep_sq = pos.getEnPassantSq();

	assert(ep_sq.isValid());
	if (ep_sq.isNotNull())
		key ^= ep_file_keys[ep_sq.getFile()];

	if (pos.getCastlingByColor(WHITE).isShortPossible())
		key ^= short_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isShortPossible())
		key ^= short_castle_keys[BLACK];

	if (pos.getCastlingByColor(WHITE).isLongPossible())
		key ^= long_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isLongPossible())
		key ^= long_castle_keys[BLACK];

	return static_cast<ZobristHash>(key);
}

#if defined(_DEBUG)
bool ZobristHash::printXOR_Diff(uint64_t key_2) {
	std::cout << (_key ^ key_2) << ' ';
	return true;
}
#endif
