#include "UCI.hpp"
#include "backend/Magic.hpp"
#include "backend/Attacks.hpp"
#include "backend/MoveGen.hpp"
#include "backend/Hash.hpp"

_INTERNAL void setupInternals() {
	SlidersAttacks::initAttackTables<Piece::BISHOP>();
	SlidersAttacks::initAttackTables<Piece::ROOK>();
}
