#include "../backend/Common.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface() = default;
	~UniversalChessInterface() = default;

	void Loop(int argc, const char* argv[]);
private:
	void Parse_UCI();

	std::string _command;
};