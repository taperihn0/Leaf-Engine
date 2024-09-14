#pragma once

#include "Common.hpp"
#include "Move.hpp"
#include "Hash.hpp"

class Game {
public:
	Game() = default;

	INLINE void recordInfo(uint64_t key, Move move) {
		assert(0 <= _idx and _idx < max_game_moves);
		_move_history[_idx] = move;
		_key_history[_idx++] = key;
	}

	INLINE Move getPrevMove(size_t halfmove_cnt) const {
		assert(0 <= halfmove_cnt and halfmove_cnt < _idx);
		return _move_history[halfmove_cnt];
	}

	INLINE uint64_t getPrevKey(size_t halfmove_cnt) const {
		assert(0 <= halfmove_cnt and halfmove_cnt < _idx);
		return _key_history[halfmove_cnt];
	}

	INLINE size_t currentHalfCount() const { return _idx; }
	INLINE void clear() { _idx = 0; }
private:
	std::array<Move, max_game_moves> _move_history;
	std::array<uint64_t, max_game_moves> _key_history;
	size_t _idx = 0;
};