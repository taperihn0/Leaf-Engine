#pragma once

#include "../backend/Position.hpp"
#include "../backend/Search.hpp"
#include "../backend/Game.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface() = default;
	~UniversalChessInterface() = default;

	void loop(int argc, const char* argv[]);
private:
	void parseUCI();
	void parseNewGame();
	void parsePosition(std::istringstream& strm);
	void parseGo(std::istringstream& strm);
	void parseIsReady();

	Position _pos;
	Search _search;
	Game _game;

	std::string _command;
};