#pragma once

#include "Common.hpp"

class Score {
public:
	INLINE Score() = default;
	INLINE Score(uint16_t val)
		: _raw(val) {}

	INLINE Score operator+(Score b) const {
		return _raw + b._raw;
	}

	INLINE Score operator+=(Score b) {
		return _raw += b._raw;
	}

	INLINE Score operator-=(Score b) {
		return _raw -= b._raw;
	}

	INLINE Score operator-(Score b) const {
		return _raw - b._raw;
	}

	INLINE bool operator>(Score b) const {
		return _raw > b._raw;
	}

	INLINE bool operator>=(Score b) const {
		return _raw >= b._raw;
	}

	INLINE bool operator<=(Score b) const {
		return _raw <= b._raw;
	}

	INLINE bool operator==(Score b) const {
		return _raw == b._raw;
	}

	INLINE bool operator!=(Score b) const {
		return _raw != b._raw;
	}

	INLINE bool operator<(Score b) const {
		return _raw < b._raw;
	}

	INLINE Score operator-() const {
		return -_raw;
	}

	INLINE int16_t toInt() const {
		return _raw;
	}

	INLINE bool isValid() const {
		return _raw != undef;
	}

	std::string toStr() const;

	static constexpr int16_t draw = 0,
		infinity = std::numeric_limits<int16_t>::max(),
		undef = infinity;
private:
	int16_t _raw;
};

inline std::string Score::toStr() const {
	if (_raw > Score::infinity - (int16_t)max_depth)
		return "mate " + std::to_string((Score::infinity - _raw + 1) / 2);
	else if (_raw < -Score::infinity + (int16_t)max_depth)
		return "mate -" + std::to_string((_raw + Score::infinity + 1) / 2);

	return "cp " + std::to_string(_raw);
}
