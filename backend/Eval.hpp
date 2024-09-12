#pragma once

#include "Position.hpp"

class Score;

class Eval {
public:
	Score matEval(const Position& pos);
	Score staticEval(const Position& pos);
private:
	Score pawnsStaticEval(const Position& pos, enumColor side);
	Score knightsStaticEval(const Position& pos, enumColor side);
	Score bishopsStaticEval(const Position& pos, enumColor side);
	Score rooksStaticEval(const Position& pos, enumColor side);
	Score queensStaticEval(const Position& pos, enumColor side);
	Score kingsStaticEval(const Position& pos, enumColor side);

	/*
		PeSTO evaluation tables provided by Chess Programming Wiki:
		https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function 
	*/

	static std::array<int16_t, 64>
		_mg_pawn_tables, 
		_mg_knight_tables, 
		_mg_bishop_tables, 
		_mg_rook_tables, 
		_mg_queen_tables, 
		_mg_king_tables;

	static std::array<int8_t, 2> _convert_factor;
};