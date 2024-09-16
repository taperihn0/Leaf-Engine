#include "Time.hpp"
#include "Search.hpp"

unsigned TimeMan::searchTime(const Position& pos, SearchLimits& limits) {
	return pos.getTurn() == WHITE ? (limits.wtime / 20 + limits.winc / 2)
								  : (limits.btime / 20 + limits.binc / 2);
}