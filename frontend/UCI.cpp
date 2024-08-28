#include "UCI.hpp"

#include <sstream>

void UniversalChessInterface::Loop(int argc, const char* argv[]) {
	std::cout << "Polish Chess Engine, " << ENGINE_NAME << " by " << AUTHOR << '\n';

	do {
		if (!std::getline(std::cin, _command))
			_command = "quit";

		std::istringstream command_stream(_command);
		std::string token;

		command_stream >> token;

		if (token == "uci") Parse_UCI();

	} while (_command != "quit");
}

inline void UniversalChessInterface::Parse_UCI() {
	std::cout << "id name " << ENGINE_NAME << '\n'
		<< "id author " << AUTHOR << '\n'
		<< "uciok" << '\n';
}