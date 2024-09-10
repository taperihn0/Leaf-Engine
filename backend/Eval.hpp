#pragma once

#include "Position.hpp"
#include "Search.hpp"

// TODO
INLINE Score staticEval(const Position& pos) {
	const enumColor turn = pos.getTurn();
	const uint16_t res = 
		(pos.getQueensBySide(turn).popCount() - pos.getQueensBySide(!turn).popCount()) * 900
		+ (pos.getRooksBySide(turn).popCount() - pos.getRooksBySide(!turn).popCount()) * 500
		+ (pos.getBishopsBySide(turn).popCount() - pos.getBishopsBySide(!turn).popCount()) * 300
		+ (pos.getKnightsBySide(turn).popCount() - pos.getKnightsBySide(!turn).popCount()) * 300
		+ (pos.getPawnsBySide(turn).popCount() - pos.getPawnsBySide(!turn).popCount()) * 100;

	return res;
}