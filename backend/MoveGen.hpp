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
		<ALL> - all pseudo-legal moves in given position
	*/

	enum enumMode : uint8_t {
		CAPTURES,
		TACTICALS,
		QUIETS,
		ALL,
	};

	template <enumMode GenType>
	static void generatePseudoLegalMoves(const Position& pos, MoveList& move_list);
};