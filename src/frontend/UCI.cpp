#include "UCI.hpp"
#include "backend/Move.hpp"
#include "backend/Search.hpp"

#include <sstream>

SearchLimits loadSearchInfo(std::istringstream& strm, std::string token) {
	SearchLimits limits;
	limits.depth = MaxDepth - 1;

	if (token == "depth") {
		strm >> std::skipws >> token;

		if (isValidNumber(token.substr(1)) and !isSigned(token)) {
			limits.depth = std::stoi(token);
		}

		strm >> std::skipws >> token;
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
	: _search(TranspositionTable(1_MB))
	, _pos(Position::StartposFEN)
{}

// ARGUMENTS AREN'T USED FOR NOW
void UniversalChessInterface::loop(int, const char*[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Polish Chess Engine, " << EngineName << " by " << Author << '\n';

	do {
		if (!std::getline(std::cin, _command))
			_command = "quit";

		std::istringstream strm(_command);
		std::string token;

		strm >> std::skipws >> token;

		if (token == "uci")				parseUCI();
		else if (token == "ucinewgame") parseNewGame();
		else if (token == "position")	parsePosition(strm);
		else if (token == "print")		_pos.print();
		else if (token == "go")			parseGo(strm);
		else if (token == "isready")	parseIsReady();
#if defined(DEBUG)
		else if (token == "see")		parseSEE(strm);
#endif

	} while (_command != "quit");
}

void UniversalChessInterface::parseUCI() {
	std::cout << "id name " << EngineName << '\n'
			  << "id author " << Author << '\n'
			  << "uciok" << '\n';
}

inline void UniversalChessInterface::parseNewGame() {
	_game.clear();
	_search.registerNewGame();
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
		// and get halfmove counter as also fullmove counter
		while (strm.rdbuf()->in_avail() != 0) {
			strm >> std::skipws >> token;
			if (token == "moves")
				break;
			given_fen += ' ' + token;
		}

		_pos.setByFEN(given_fen);
	}
	else if (token == "startpos") {
		_pos.setStartingPos();
		_game.clear();
	}
	
	if (token != "moves")
		strm >> std::skipws >> token;

	if (token == "moves") {
		while (strm >> std::skipws >> token) {
			Move32b move = Move32b::fromStr<Move32b::Notation::PURE>(_pos, token);

			_game.recordInfo(_pos.getZobristKey(), move);
			_pos.make(move);
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

#if defined (DEBUG)
void UniversalChessInterface::parseSEE(std::istringstream& strm) {
	std::string os, ds;
	strm >> std::skipws >> os >> std::skipws >> ds;
	Square org = Square::fromChar(os[0], os[1]);
	Square dst = Square::fromChar(ds[0], ds[1]);
	int score = _pos.StaticExchangeEval<false>(org, dst, _pos.pieceOn(dst, _pos.getOppositeTurn()), 
											  _pos.pieceOn(org, _pos.getTurn()));
	std::cout << score << std::endl;
}
#endif
