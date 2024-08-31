#pragma once

#include "Position.hpp"
#include "MoveList.hpp"

class MoveGenerator {
public:
	enum enumMode {
		CAPTURES, 
		QUIETS
	};

	template <enumMode GenType>
	static void generatePseudoLegalMoves(const Position pos, MoveList& move_list);
};