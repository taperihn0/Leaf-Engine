#pragma once

#include "Common.hpp"

enum enumColor : bool {
	WHITE, BLACK
};

INLINE constexpr enumColor operator!(enumColor opp) {
	return static_cast<enumColor>(!static_cast<bool>(opp));
}

class Turn {
public:
	Turn() = default;
	INLINE constexpr Turn(enumColor c)
		: _col(c) {}

	INLINE constexpr operator enumColor() const {
		return _col;
	}

	INLINE constexpr Turn operator=(enumColor c) {
		return _col = c;
	}

	INLINE constexpr Turn operator!() const {
		return !_col;
	}

	void fromChar(char c) {
		_col = c == 'w' ? WHITE : BLACK;
	}

	void print() const {
		std::cout << (_col == WHITE ? 'w' : 'b');
	}

private:
	enumColor _col;
};
