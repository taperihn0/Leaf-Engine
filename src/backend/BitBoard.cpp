#include "BitBoard.hpp"

void BitBoard::print(std::ostream& os) const {
	for (int h = 7; h >= 0; h--) {
		for (int i = h * 8; i < (h + 1) * 8; i++)
			os << static_cast<bool>((1_ui64 << i) & _board);
		os << '\n';
	}
}

RectangularTable& RectangularTable::get() {
	static RectangularTable RecTable;
	return RecTable;
}

BitBoard RectangularTable::inBetweenOnFly(Square org, Square dst) {
	BitBoard res = BitBoard(0_ui64);

	const Square sq_min = std::min(org, dst),
				 sq_max = std::max(org, dst);

	// loop approach for each case: through file, rank and diagonal
	if (org % 8 == dst % 8) {
		for (int i = 0; i <= std::abs(dst / 8 - org / 8); i++) {
			res.setBit((sq_min / 8 + i) * 8 + org % 8);
		}
	}
	else if (org / 8 == dst / 8) {
		for (int i = 0; i <= std::abs(dst % 8 - org % 8); i++) {
			res.setBit(sq_min + i);
		}
	}
	else if (std::abs(org % 8 - dst % 8) == std::abs(org / 8 - dst / 8)) {
		for (int i = 0; i <= std::abs(org % 8 - dst % 8); i++) {
			res.setBit(sq_min + i * (sq_min % 8 < sq_max % 8 ? 9 : 7));
		}
	}

	// if there is no straight path between org and dst, return Universe
	return res.isEmpty() ? BitBoard(BitBoard::Universe) : res;
}

RectangularTable::RectangularTable() {
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			t64[i][j] = inBetweenOnFly(i, j);
		}
	}
}
