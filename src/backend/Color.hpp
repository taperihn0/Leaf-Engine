#pragma once

#include "Common.hpp"

enum enumColor : bool {
	WHITE = 0, 
	BLACK
};

_NODISCARD _INLINE constexpr enumColor operator!(enumColor opp) {
	return static_cast<enumColor>(!static_cast<bool>(opp));
}

class Turn {
public:
	_INLINE Turn() = default;
	
	_INLINE constexpr Turn(enumColor c)
		: _col(c) {}

	_NODISCARD _INLINE constexpr operator enumColor() const {
		return _col;
	}

	_INLINE constexpr Turn operator=(enumColor c) {
		return _col = c;
	}

	_NODISCARD _INLINE constexpr Turn operator!() const {
		return !_col;
	}

	_INLINE void fromChar(char c) {
		_col = c == 'w' ? WHITE : BLACK;
	}

	void print(std::ostream& os = std::cout) const {
		os << (_col == WHITE ? 'w' : 'b');
	}
private:
	enumColor _col;
};
