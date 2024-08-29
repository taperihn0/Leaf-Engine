#include "../backend/Common.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface() = default;
	~UniversalChessInterface() = default;

	void loop(int argc, const char* argv[]);
private:
	void parse_UCI();

	std::string _command;
};