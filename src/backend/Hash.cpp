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

#include "Hash.hpp"
#include "Position.hpp"

#include <random>

ZobristMasks& ZobristMasks::get() {
	static ZobristMasks ZKeys;
	return ZKeys;
}

ZobristMasks::ZobristMasks() {
	fillKeys();
}

void ZobristMasks::fillKeys() {
	static auto get_sparse_random_u64 = []() {
		return sparseRandom<uint64_t>(1, maxof<uint64_t>());
	};

	for (int sq = 0; sq < 64; sq++) {
		for (enumColor col : { WHITE, BLACK }) {
			for (auto piece_t : Piece::PieceTypeList) {
				piece_keys[col][piece_t][sq] = get_sparse_random_u64();
			}
		}
	}

	black_key = get_sparse_random_u64();

	for (int file = 0; file < 8; file++) {
		ep_file_keys[file] = get_sparse_random_u64();
	}

	for (enumColor col : { WHITE, BLACK }) {
		short_castle_keys[col] = get_sparse_random_u64();
		long_castle_keys[col] = get_sparse_random_u64();
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
bool ZHash::printXOR_Diff(uint64_t key_2) {
	std::cout << (_key ^ key_2) << ' ';
	return true;
}
#endif
