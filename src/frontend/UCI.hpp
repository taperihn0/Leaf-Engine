#pragma once

#include "backend/Position.hpp"
#include "backend/Search.hpp"
#include "backend/Game.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface();
	~UniversalChessInterface() = default;

	static SearchLimits loadSearchLimits(std::istringstream& strm, std::string token);

	void loop(int argc, const char* argv[]);
private:
	void parseUCI();
	void parseNewGame();
	void parsePosition(std::istringstream& strm);
	void parseGo(std::istringstream& strm);
	void parseIsReady();
#if defined (DEBUG)
	void parseSEE(std::istringstream& strm);
#endif

	Search _search;
	Position _pos;
	FullInfoRecord _game;

	std::string _command;
};