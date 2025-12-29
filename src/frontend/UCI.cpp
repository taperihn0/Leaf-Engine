#include "UCI.hpp"
#include "backend/Move.hpp"
#include "backend/Search.hpp"
#include "PackedNetwork.hpp"
#include "Accumulator.hpp"
#include "NetworkEval.hpp"

#include <sstream>

SearchLimits UniversalChessInterface::loadSearchLimits(std::istringstream& strm, std::string token) {
	SearchLimits limits;
	limits.depth = MaxDepth - 1;
    limits.nodes = 0;

#define TERMINATE_READ_IF_EMPTY(str, res) \
    do { if ((str).empty()) return (res); } while(false);

    while (strm.rdbuf()->in_avail() > 0) {

        if (token == "depth") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

        	if (isValidUnsigned(token))
        		limits.depth = std::stoi(token);
        }
        else if (token == "wtime") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

            if (isValidUnsigned(token))
                limits.wtime = std::stoi(token);
        }
        else if (token == "btime") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

            if (isValidUnsigned(token))
                limits.btime = std::stoi(token);
        }
        else if (token == "winc") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

            if (isValidUnsigned(token))
                limits.winc = std::stoi(token);
        }
        else if (token == "binc") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

            if (isValidUnsigned(token))
        	    limits.binc = std::stoi(token);
        }
        else if (token == "nodes") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);
            
            if (isValidUnsigned(token))
                limits.nodes = std::stoi(token);
        }
        // Custom option
        else if (token == "qnodes") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

            if (isValidUnsigned(token))
                limits.qnodes = std::stoi(token);
        }

        strm >> std::skipws >> token;
    }

#undef TERMINATE_READ_IF_EMPTY

	return limits;
}

UniversalChessInterface::UniversalChessInterface()
	: _search(TranspositionTable(1_MB))
	, _pos(std::string(Position::StartposFEN))
{}

// ARGUMENTS AREN'T USED FOR NOW
void UniversalChessInterface::loop(int, const char*[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Polish Chess Engine, " << EngineName << " by " << Author << '\n';

	std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

		if (token == "uci")					parseUCI();
		else if (token == "ucinewgame") 	parseNewGame();
		else if (token == "position")		parsePosition(strm);
		else if (token == "print")			_pos.print();
		else if (token == "go")				parseGo(strm);
		else if (token == "isready")		parseIsReady();
		else if (token == "export_net") 	parseNet(strm);
		else if (token == "rewrite_header") parseRewriteNet(strm);

#if defined(DEBUG)
		else if (token == "see")			parseSEE(strm);
#endif

	} while (command != "quit");
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
		while (strm.rdbuf()->in_avail() > 0) {
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
	
	SearchLimits limits = loadSearchLimits(strm, token);
	_search.bestMove(_pos, _game, limits);
}

inline void UniversalChessInterface::parseIsReady() {
	std::cout << "readyok\n";
}

#if defined(DEBUG)
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

void UniversalChessInterface::parseNet(std::istringstream& strm) {
	std::string path;
	strm >> std::skipws >> path;

	if (path == "default")
		nn::GlobPackedNetwork.loadDefaultNet();
	else
		nn::GlobPackedNetwork.loadFromFile(path);
}

void UniversalChessInterface::parseRewriteNet(std::istringstream& strm) {
	std::string in_path, out_path;
	strm >> std::skipws >> in_path >> std::skipws >> out_path;

	nn::PackedNeuralNetwork::Header header{};
	// modify it manually
	header.dual_hl = true;
	header.layer_size[0] = nn::NetworkInputSize;
	header.layer_size[1] = nn::NetworkHiddenLayerSize;
	header.layer_size[2] = 1;
	header.layer_count = 3;
	
	nn::PackedNeuralNetwork::rewriteWithHeader(in_path, out_path, header);
}
