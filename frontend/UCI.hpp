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

	bool isValidNumber(const std::string& str);
	bool isSigned(const std::string& str);

	Position _pos;

	std::string _command;
};