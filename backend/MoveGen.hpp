#pragma once

#include "Position.hpp"
#include "MoveList.hpp"

class MoveGenerator {
public:

	/*
		Generation mode:
		<CAPTURES> - all pseudo-legal captures and queen promotions
		<TACTICALS> - all pseudo-legal captures and all promotions
		<QUIETS> - all pseudo-legal non-captures and promotions without queen promotions
	*/

	enum enumMode {
		CAPTURES,
		TACTICALS,
		QUIETS
	};

	template <enumMode GenType>
	static void generatePseudoLegalMoves(const Position& pos, MoveList& move_list);
};