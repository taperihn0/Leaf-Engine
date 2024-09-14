#include "UCI.hpp"
#include "../backend/Move.hpp"

#include <sstream>

void UniversalChessInterface::loop(int argc, const char* argv[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Polish Chess Engine, " << ENGINE_NAME << " by " << AUTHOR << '\n';

	do {
		if (!std::getline(std::cin, _command))
			_command = "quit";

		std::istringstream strm(_command);
		std::string token;

		strm >> std::skipws >> token;

		if (token == "uci") parseUCI();
		else if (token == "ucinewgame") parseNewGame();
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

void UniversalChessInterface::parseNewGame() {
	_game.clear();
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
		_game.clear();
	}
		
	strm >> std::skipws >> token;
	if (token == "moves") {
		Position::IrreversibleState unused;

		while (strm >> std::skipws >> token) {
			Move move = Move::fromStr(_pos, token);

			_game.recordInfo(_pos.getZobristKey(), move);
			_pos.make(move, unused);
		}
	}
}

void UniversalChessInterface::parseGo(std::istringstream& strm) {
	std::string token;
	strm >> std::skipws >> token;

	if (token == "perft") {
		strm >> std::skipws >> token;

		if (isValidNumber(token.substr(1)) and !isSigned(token)) {
			const unsigned depth = std::stoi(token);
			_pos.perft(depth);
		}
	}
	else if (token == "depth") {
		strm >> std::skipws >> token;

		if (isValidNumber(token.substr(1)) and !isSigned(token)) {
			const unsigned depth = std::stoi(token);
			_search.bestMove(_pos, _game, depth);
		}
	}
}

INLINE bool UniversalChessInterface::isValidNumber(const std::string& str) {
	return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

INLINE bool UniversalChessInterface::isSigned(const std::string& str) {
	return !str.empty() and str[0] == '-';
}
