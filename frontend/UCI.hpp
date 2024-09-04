#pragma once

#include "../backend/Position.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface() = default;
	~UniversalChessInterface() = default;

	void loop(int argc, const char* argv[]);
private:
	void parseUCI();
	void parsePosition(std::istringstream& strm);
	void parseGo(std::istringstream& strm);

	Position _pos;

	std::string _command;
};