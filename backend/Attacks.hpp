#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"

BitBoard kingAttacks(Square sq) {
	BitBoard bit(sq);
	return nortOne(bit) | noEaOne(bit) | eastOne(bit)
		| soEaOne(bit) | soutOne(bit) | soWeOne(bit)
		| westOne(bit) | noWeOne(bit);
}

BitBoard whitePawnAttacks(Square sq) {
	BitBoard bit(sq);
	return noEaOne(bit) | noWeOne(bit);
}

BitBoard blackPawnAttacks(Square sq) {
	BitBoard bit(sq);
	return soEaOne(bit) | soWeOne(bit);
}

BitBoard knightAttacks(Square sq) {
	BitBoard bit(sq);
	return noNoEa(bit) | noEaEa(bit) | soEaEa(bit)
		| soSoEa(bit) | soSoWe(bit) | soWeWe(bit)
		| noWeWe(bit) | noNoWe(bit);
}