#pragma once

#include "Position.hpp"
#include "MoveList.hpp"

class MoveGenerator {
public:

	/*
	*	Generation mode:
	*	<CAPTURES> - all pseudo-legal captures and queen promotions
	*	<TACTICALS> - all pseudo-legal captures and all promotions
	*	<QUIETS> - all pseudo-legal non-captures and promotions without queen promotions
	*	<ALL> - all pseudo-legal moves in given position
	*/

	enum enumMode : uint8_t {
		CAPTURES,
		TACTICALS,
		QUIETS,
		ALL,
	};

	template <enumMode GenType>
	static void generatePseudoLegalMoves(const Position& pos, MoveList& move_list);

    template <enumMode GenType>
    static void generateLegalMoves(Position& pos, MoveList& move_list);

	template <enumMode GenType>
	_NODISCARD static Move32b getRandomLegalMove(Position& pos);

	_NODISCARD static bool isAnyCapture(Position& pos);
};

// helpful alias for MoveGenerator class
using MoveGen = MoveGenerator;
