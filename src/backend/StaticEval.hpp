#pragma once

#include "Position.hpp"
#include "Score.hpp"

/* 
*  Static evaluation utilities.
*/

inline _P_CONSTEXPR int PawnValue   = 100;
inline _P_CONSTEXPR int KnightValue = 300;
inline _P_CONSTEXPR int BishopValue = 300;
inline _P_CONSTEXPR int RookValue   = 500;
inline _P_CONSTEXPR int QueenValue  = 900;

class StaticEval {
public:
	static Score evaluateEndgame(const Position& pos);
	static Score matEval(const Position& pos);
	static Score staticEval(const Position& pos);
private:
	static Score pawnsStaticEval(const Position& pos, enumColor side);
	static Score knightsStaticEval(const Position& pos, enumColor side);
	static Score bishopsStaticEval(const Position& pos, enumColor side);
	static Score rooksStaticEval(const Position& pos, enumColor side);
	static Score queensStaticEval(const Position& pos, enumColor side);
	static Score kingsStaticEval(const Position& pos, enumColor side);

	static std::array<int16_t, 64>
		_mg_pawn_tables, 
		_mg_knight_tables, 
		_mg_bishop_tables, 
		_mg_rook_tables, 
		_mg_queen_tables, 
		_mg_king_tables;
};
