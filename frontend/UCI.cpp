#include "UCI.hpp"
#include "../backend/Move.hpp"
#include "../backend/Search.hpp"

#include <sstream>

SearchLimits loadSearchInfo(std::istringstream& strm, std::string token) {
	SearchLimits limits;
	limits.depth = max_depth - 1;

	if (token == "depth") {
		strm >> std::skipws >> token;

		if (isValidNumber(token.substr(1)) and !isSigned(token)) {
			limits.depth = std::stoi(token);
		}
	}

	if (token == "wtime") {
		strm >> std::skipws >> token;
		limits.wtime = std::stoi(token);

		strm >> std::skipws >> token >> std::skipws >> token;
		limits.btime = std::stoi(token);

		strm >> std::skipws >> token >> std::skipws >> token;
		limits.winc = std::stoi(token);

		strm >> std::skipws >> token >> std::skipws >> token;
		limits.binc = std::stoi(token);
	}

	return limits;
}

UniversalChessInterface::UniversalChessInterface()
	: _search(_tt) {}

void UniversalChessInterface::loop(int argc, const char* argv[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Polish Chess Engine, " << ENGINE_NAME << " by " << AUTHOR << '\n';

	//_pos.setByFEN("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1");
	//std::cout << _pos.StaticExchangeEval(Square::e5) << '\n';

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
		else if (token == "isready") parseIsReady();

	} while (_command != "quit");
}

void UniversalChessInterface::parseUCI() {
	std::cout << "id name " << ENGINE_NAME << '\n'
		<< "id author " << AUTHOR << '\n'
		<< "uciok" << '\n';
}

inline void UniversalChessInterface::parseNewGame() {
	_game.clear();
	MoveOrder<STAGED>::clearCounterMoveHistory();
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

inline void UniversalChessInterface::parseGo(std::istringstream& strm) {
	std::string token;
	strm >> std::skipws >> token;

	if (token == "perft") {
		strm >> std::skipws >> token;

		if (isValidNumber(token.substr(1)) and !isSigned(token)) {
			const unsigned depth = std::stoi(token);
			_pos.perft(depth);
		}

		return;
	}
	
	SearchLimits limits = loadSearchInfo(strm, token);
	_search.bestMove(_pos, _game, limits);
}

inline void UniversalChessInterface::parseIsReady() {
	std::cout << "readyok\n";
}
