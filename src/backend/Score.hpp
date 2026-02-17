#pragma once

#include "Common.hpp"

class Score {
public:
	using int_t = int16_t;

	INLINE Score() = default;
	INLINE constexpr Score(int_t val) 
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

	INLINE Score operator*(Score b) const {
		return _raw * b._raw;
	}

	INLINE Score operator/(Score b) const {
		return _raw / b._raw;
	}

	INLINE Score operator-() const {
		return -_raw;
	}

	INLINE explicit operator int() const {
		return _raw;
	}

	INLINE explicit operator float() const {
		return static_cast<float>(_raw);
	}

	INLINE bool isValid() const {
		return _raw != Undef and _raw != -Undef;
	}

	INLINE bool isMateScore() const {
		return isValid() and (_raw > MateBound or _raw < -MateBound);
	}

	std::string toStr() const;
	
	static constexpr int_t Draw		  = 0,
						   Mate		  = 32000,
						   MateBound  = Mate - MaxDepth,
						   Infinity   = std::numeric_limits<int_t>::max(),
						   Undef      = 32500;
private:
	int_t				   _raw;
};

_INTERNAL std::string Score::toStr() const {
	if (_raw > MateBound)
		return "mate " + std::to_string((Score::Mate - _raw + 1) / 2);
	else if (_raw < -MateBound)
		return "mate -" + std::to_string((_raw + Score::Mate + 1) / 2);

	return "cp " + std::to_string(_raw);
}
