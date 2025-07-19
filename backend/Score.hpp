#pragma once

#include "Common.hpp"

class Score {
public:
	using int_t = int16_t;

	INLINE Score() = default;
	INLINE Score(int_t val)
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

	INLINE int_t toInt() const {
		return _raw;
	}

	INLINE bool isValid() const {
		return _raw != Undef and _raw != -Undef;
	}

	std::string toStr() const;
	
	static constexpr int_t Draw		= 0,
						   MateBound = 30000,
						   Mate		= 32000,
						   Infinity   = std::numeric_limits<int_t>::max(),
						   Undef      = 32500;
private:
	int_t				   _raw;
};

inline std::string Score::toStr() const {
	if (_raw > MateBound)
		return "Mate " + std::to_string((Score::Mate - _raw + 1) / 2);
	else if (_raw < -MateBound)
		return "Mate -" + std::to_string((_raw + Score::Mate + 1) / 2);

	return "cp " + std::to_string(_raw);
}
