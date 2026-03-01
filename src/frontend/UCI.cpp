#include "UCI.hpp"
#include "backend/Move.hpp"
#include "backend/Search.hpp"
#include "PackedNetwork.hpp"
#include "Accumulator.hpp"
#include "NetworkEval.hpp"
#include "Tuning.hpp"

#include <sstream>

UniversalChessInterface::Options UniversalChessInterface::_options = { 
		// --- Regular parameters
		OptionHash(SpinType<ll>(1, 1, 512)), 
		OptionClearHash(),
		{ // --- Tunable parameters
		OptionTunableParam(SpinType<double>(MaxQuietsHistoryPow, 12.,  14.),    "MaxQuietsHistoryPow",  1.2),
		OptionTunableParam(SpinType<double>(IidDepth,            2.,   5.),     "IidDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(IidDepthDiv,         8.,   14.),    "IidDepthDiv", 		    2.8),
		OptionTunableParam(SpinType<double>(RfpDepth,     	     2.,   4.),     "RfpDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(RazorDepth,          2.,   4.),     "RazorDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(FutilityDepth,       2.,   5.),     "FutilityDepth", 	    2.8),
		OptionTunableParam(SpinType<double>(LmrDepth,     	     1.,   4.),     "LmrDepth", 			2.9),
		OptionTunableParam(SpinType<double>(NullReduction,       15.,  36.),    "NullReduction", 		1.3),
		OptionTunableParam(SpinType<double>(LmrMoveCount,        1.,   16.),    "LmrMoveCount", 		0.7),
		OptionTunableParam(SpinType<double>(RazorMultDelta,      5.,   40.),    "RazorMultDelta", 		0.43),
		OptionTunableParam(SpinType<double>(RfpMultDelta,        10.,  220.),   "RfpMultDelta", 		0.51),
		OptionTunableParam(SpinType<double>(FutilityMoveCount,   1.,   16.),    "FutilityMoveCount", 	0.61),
		OptionTunableParam(SpinType<double>(FutilityDelta,       2.,   50.),    "FutilityDelta", 		0.85),
		OptionTunableParam(SpinType<double>(RazorBaseDelta,      20.,  500.),   "RazorBaseDelta", 		0.4),
		OptionTunableParam(SpinType<double>(QMaterialDelta,      700., 1200.),  "QMaterialDelta", 		0.45),
		OptionTunableParam(SpinType<double>(QProbeDepth,         -5.,  1.),     "QProbeDepth", 			1.3),
		//OptionTunableParam(SpinType<double>(ContemptFactor,      0.,   50.),    "ContemptFactor", 1.),
		OptionTunableParam(SpinType<double>(NNEvalScale,         7.,   12.),    "NNEvalScale", 			1.5),
		OptionTunableParam(SpinType<double>(ImprovingRate, 		 30.,  90.),    "ImprovingRate" , 	  	0.7),
		OptionTunableParam(SpinType<double>(RfpImprovingSink, 	 1.,   10.),    "RfpImprovingSink",   	1.4),
		OptionTunableParam(SpinType<double>(NullMargin, 		 3.,   50.),    "NullMargin", 			0.3),
		OptionTunableParam(SpinType<double>(NullImprovingSink ,  1.,   10.), 	"NullImprovingSink", 	1.4),
		OptionTunableParam(SpinType<double>(DynImprovementDepth, 6.,   16.),    "DynImprovementDepth",  1.6),
		OptionTunableParam(SpinType<double>(TTEvalCorrRate, 	 1.,   3.),     "TTEvalCorrRate", 		2.2),
		OptionTunableParam(SpinType<double>(NullDiffScale, 	 	 800., 1300.),  "NullDiffScale", 		0.23),
		OptionTunableParam(SpinType<double>(NullDepth, 	 	     2.,   5.),     "NullDepth", 		    2.7),
		OptionTunableParam(SpinType<double>(NextDepthTimeRed ,   4.,   8.), 	"NextDepthTimeRed",     2.5),
		OptionTunableParam(SpinType<double>(UnstableMatMargin,   10.,  80.), 	"UnstableMatMargin",    2.1),
		OptionTunableParam(SpinType<double>(UnstableMultMargin,  4.,   12.), 	"UnstableMultMargin",   2.7),
		OptionTunableParam(SpinType<double>(MinTimeBranchFactor, 1.,   2.), 	"MinTimeBranchFactor",  2.),
		OptionTunableParam(SpinType<double>(MaxTimeBranchFactor, 3.,   5.), 	"MaxTimeBranchFactor",  2.),
		OptionTunableParam(SpinType<double>(KnightCapturedScore, 260., 350.),   "KnightCapturedScore",  0.8),
		OptionTunableParam(SpinType<double>(BishopCapturedScore, 260., 350.),   "BishopCapturedScore",  0.8),
		OptionTunableParam(SpinType<double>(ToKnightPromoScore , 80.,  300.),   "ToKnightPromoScore",   1.3),
		OptionTunableParam(SpinType<double>(ToBishopPromoScore , 50.,  300.),   "ToBishopPromoScore",   1.3),
		OptionTunableParam(SpinType<double>(ToRookPromoScore   , 150., 500.),   "ToRookPromoScore",     1.3),
		OptionTunableParam(SpinType<double>(ToQueenPromoScore  , 700., 1020.),  "ToQueenPromoScore",    1.3),
		OptionTunableParam(SpinType<double>(ContemptDiv        , 30.,  160.),   "ContemptDiv",          1.8),
		}, 
};

SearchLimits UniversalChessInterface::loadSearchLimits(std::istringstream& strm, std::string token) {
	SearchLimits limits;
	limits.depth = MaxDepth;
    limits.nodes = 0;

#define TERMINATE_READ_IF_EMPTY(str, res) \
    do { if ((str).empty()) return (res); } while(false);

    while (strm.rdbuf()->in_avail() > 0) {

        if (token == "depth") {
            strm >> std::skipws >> token;
            TERMINATE_READ_IF_EMPTY(token, limits);

        	if (isValidUnsigned(token))
        		limits.depth = std::min(limits.depth, std::stoi(token));
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
	, _pos(StartposFEN)
{}

// ARGUMENTS AREN'T USED FOR NOW
void UniversalChessInterface::loop(int, const char*[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

#if defined(_ENABLE_TUNING)
	GlobParamMapping.createMapping();
#endif

	std::cout << "Polish Chess Engine, " << EngineName << " by " << Author << '\n';

	std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

			 if (token == "uci")			parseUCI();
		else if (token == "ucinewgame") 	parseNewGame();
		else if (token == "position")		parsePosition(strm);
		else if (token == "print")			_pos.print();
		else if (token == "go")				parseGo(strm);
		else if (token == "isready")		parseIsReady();
		else if (token == "export_net") 	parseNet(strm);
		else if (token == "rewrite_header") parseRewriteNet(strm);
		else if (token == "options")		parseShowOptions();
		else if (token == "setoption")		parseSetOptions(strm);

#if defined(_UCI_DEBUG_UTILS)
		else if (token == "see")			parseSEE(strm);
		else if (token == "nneval")			parseNNEval(strm);
#endif

	} while (command != "quit");
}

std::vector<OptionTunableParam>& UniversalChessInterface::getTunableOptions() {
	return _options.tunable_params;
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
		_game.clear();
	}
	else if (token == "startpos") {
		_pos.setStartingPos();
		_game.clear();
	}
	else if (token == "kiwipete") {
		_pos.setByFEN(static_cast<std::string>(KiwipeteFEN));
		_game.clear();
	}
	
	if (token != "moves")
		strm >> std::skipws >> token;

	if (token == "moves") {
		while (strm >> std::skipws >> token) {
			Move32b move = Move32b::fromStr<Move32b::Notation::REGULAR>(_pos, token);
			_game.recordInfo(_pos.getZobristKey(), move);
			_pos.make(move);
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

		return;
	}
	
	SearchLimits limits = loadSearchLimits(strm, token);
	_search.bestMove(_pos, _game, limits);
}

void UniversalChessInterface::parseIsReady() {
	std::cout << "readyok\n";
}

#if defined(_UCI_DEBUG_UTILS)

void UniversalChessInterface::parseSEE(std::istringstream& strm) {
	std::string os, ds;
	strm >> std::skipws >> os >> std::skipws >> ds;
	Square org = Square::fromChar(os[0], os[1]);
	Square dst = Square::fromChar(ds[0], ds[1]);
	int score = _pos.StaticExchangeEval<false>(org, dst, _pos.pieceOn(dst, _pos.getOppositeTurn()), 
											  _pos.pieceOn(org, _pos.getTurn()));
	std::cout << score << std::endl;
}

void UniversalChessInterface::parseNNEval(std::istringstream& strm) {
	std::string fen;
	std::getline(strm >> std::ws, fen);

	Position pos;

	if (fen == "startpos")
		pos.setStartingPos();
	else
		pos.setByFEN(fen);

	const Score score = nn::NEval::evaluate(nn::GlobPackedNetwork, pos);

	std::cout << static_cast<int>(score) << std::endl;
}

#endif

void UniversalChessInterface::parseNet(std::istringstream& strm) {
	std::string path;
	strm >> std::skipws >> path;

	bool status;

	if (path == "default")
		status = nn::GlobPackedNetwork.loadDefaultNet();
	else
		status = nn::GlobPackedNetwork.loadFromFile(path);

	if (status)
		std::cout << "Net loaded successfully" << std::endl;
	else
		std::cout << "Failed to load given net" << std::endl;
}

void UniversalChessInterface::parseRewriteNet(std::istringstream& strm) {
	std::string in_path, out_path;
	strm >> std::skipws >> in_path >> std::skipws >> out_path;

	nn::PackedNeuralNetwork::Header header{};
	// modify it manually
	header.dual_hl = nn::NetworkDualHiddenLayer;
	header.layer_size[0] = nn::NetworkInputSize;
	header.layer_size[1] = nn::NetworkHiddenLayerSize;
	header.layer_size[2] = nn::NetworkOutputScale;
	header.layer_count = nn::NetworkLayerCount;
	
	bool status = nn::PackedNeuralNetwork::rewriteWithHeader(in_path, out_path, header);

	if (status)
		std::cout << "Rewrited succesfully." << std::endl;
	else
		std::cout << "Rewrite failed." << std::endl;
}

void UniversalChessInterface::parseSetOptions(std::istringstream& strm) {
	std::string option, token;
	strm >> std::skipws >> token >> std::skipws >> option;

	if (option == "Clear") {
		strm >> std::skipws >> token;

		if (token == "Hash") {
			_search.clearHashTT();
		}
	}
	
	if (option == "Hash") {
		strm >> std::skipws >> token;

		if (token == "value") {
			strm >> std::skipws >> token;
			
			const ll val = std::stoi(token);

			_options.hash.set(val);
			_search.resizeHashTT(_options.hash.getCurrentValue() * 1_MB);
		}
	}

#if defined(_ENABLE_TUNING)

	for (OptionTunableParam& param : _options.tunable_params) {
		if (option == param.str) {
			strm >> std::skipws >> token;

			if (token == "value") {
				strm >> std::skipws >> token;

				const double val = std::stod(token);
				param.set(val);
				
				int* const addr = reinterpret_cast<int*>(GlobParamMapping.getAddressOf(param.str));				
				ASSERT(addr, "Invalid static-address to tunable parameter");

				*addr = static_cast<int>(std::round(val));
				break;
			}
		}
	}

#endif
}

void UniversalChessInterface::parseShowOptions() {
	_options.hash.print();
	_options.clear_hash.print();

#if defined(_ENABLE_TUNING)
	for (OptionTunableParam& param : _options.tunable_params) {
		param.print();
	}
#endif
}
