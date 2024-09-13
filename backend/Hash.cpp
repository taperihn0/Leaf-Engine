#include "Hash.hpp"
#include "Position.hpp"

#include <random>

static constexpr int random_seed = 0xfff;

INLINE uint64_t randomU64() {
	static std::mt19937_64 engine(random_seed);
	static std::uniform_int_distribution<uint64_t> dist(1, std::numeric_limits<uint64_t>::max());
	return dist(engine) & dist(engine);
}

void ZobristHash::fillKeys() {
	for (int sq = 0; sq < 64; sq++) {
		for (enumColor col : { WHITE, BLACK }) {
			for (auto piece_t : Piece::piece_list) {
				_piece_keys[col][piece_t][sq] = randomU64();
			}
		}
	}

	_black_key = randomU64();

	for (int file = 0; file < 8; file++) {
		_ep_file_keys[file] = randomU64();
	}

	for (enumColor col : { WHITE, BLACK }) {
		_short_castle_keys[col] = randomU64();
		_long_castle_keys[col] = randomU64();
	}
}

uint64_t ZobristHash::generateOnFly(const Position& pos) {
	uint64_t key = 0;

	for (int sq = 0; sq < 64; sq++) {
		const Piece piece = pos.pieceOn(sq);

		if (piece.getType() != Piece::NONE)
			key ^= _piece_keys[piece.getColor()][piece.getType()][sq];
	}

	if (pos.getTurn() == BLACK)
		key ^= _black_key;

	const Square ep_sq = pos.getEnPassantSq();

	assert(ep_sq.isValid());
	if (ep_sq.isNotNull())
		key ^= _ep_file_keys[ep_sq.getFile()];

	if (pos.getCastlingByColor(WHITE).isShortPossible())
		key ^= _short_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isShortPossible())
		key ^= _short_castle_keys[BLACK];

	if (pos.getCastlingByColor(WHITE).isLongPossible())
		key ^= _long_castle_keys[WHITE];
	if (pos.getCastlingByColor(BLACK).isLongPossible())
		key ^= _long_castle_keys[BLACK];

	return key;
}

#if defined(_DEBUG)
bool ZobristHash::printXOR_Diff(uint64_t key_2) {
	std::cout << (key ^ key_2) << ' ';
	return true;
}
#endif
