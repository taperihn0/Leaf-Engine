#include "UCI.hpp"
#include "../backend/Move.hpp"

#include <sstream>

void UniversalChessInterface::loop(int argc, const char* argv[]) {
	std::cout << "Polish Chess Engine, " << ENGINE_NAME << " by " << AUTHOR << '\n';

	do {
		if (!std::getline(std::cin, _command))
			_command = "quit";

		std::istringstream strm(_command);
		std::string token;

		strm >> std::skipws >> token;

		if (token == "uci") parseUCI();
		else if (token == "position") parsePosition(strm);
		else if (token == "print") _pos.print();
		else if (token == "go") parseGo(strm);

	} while (_command != "quit");
}

void UniversalChessInterface::parseUCI() {
	std::cout << "id name " << ENGINE_NAME << '\n'
		<< "id author " << AUTHOR << '\n'
		<< "uciok" << '\n';
}

void UniversalChessInterface::parsePosition(std::istringstream& strm) {
	std::string token;
	strm >> std::skipws >> token;

	if (token == "fen") {
		std::string given_fen;

		// load piece distribution
		strm >> std::skipws >> token;
		given_fen += token;

		// get side to move, castling rights, en passant square
		// and get halfmove counter as fullmove counter
		for (int i = 0; i < 5; i++) {
			strm >> std::skipws >> token;
			given_fen += ' ' + token;
		}

		_pos.setByFEN(given_fen);
	}
	else if (token == "startpos") {
		_pos.setStartingPos();
	}

	strm >> std::skipws >> token;
	if (token == "moves") {
		// TODO
		while (strm >> std::skipws >> token) {
			Move move = Move::fromStr(_pos, token);
			Position::IrreversibleState tmp;
			_pos.make(move, tmp);
		}
	}
}

void UniversalChessInterface::parseGo(std::istringstream& strm) {
	std::string token;
	strm >> std::skipws >> token;

	if (token == "perft") {
		strm >> std::skipws >> token;
		const int depth = std::stoi(token);
		_pos.perft(depth);
	}
}

