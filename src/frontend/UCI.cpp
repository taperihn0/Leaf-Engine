#include "UCI.hpp"
#include "backend/Move.hpp"
#include "backend/Search.hpp"
#include "PackedNetwork.hpp"
#include "Accumulator.hpp"
#include "NetworkEval.hpp"
#include "Tuning.hpp"
#include "utils/Sets.hpp"
#include "Tablebase.hpp"

#if defined(_MSC_VER)
#include <io.h>
#include <fcntl.h>
#endif

#include <sstream>

UniversalChessInterface::Options UniversalChessInterface::_options = { 
		// --- Regular parameters ---
		OptionHash(SpinType<ll>(1, 1, 512)), 
		OptionClearHash(),
		OptionPath(StringType("<empty>")),
		{ // --- Tunable parameters ---
		OptionTunableParam(SpinType<double>(IidDepth,					  2.,   5.),    "IidDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(IidDepthDiv,				  8.,   14.),   "IidDepthDiv", 		    2.8),
		OptionTunableParam(SpinType<double>(RfpDepth,     				  2.,   4.),    "RfpDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(RazorDepth,					  2.,   4.),    "RazorDepth", 		    2.8),
		OptionTunableParam(SpinType<double>(FutilityDepth,				  2.,   5.),    "FutilityDepth", 	    2.8),
		OptionTunableParam(SpinType<double>(LmrDepth,     				  1.,   4.),    "LmrDepth", 			2.9),
		OptionTunableParam(SpinType<double>(NullReduction,				  15.,  36.),   "NullReduction", 		1.3),
		OptionTunableParam(SpinType<double>(LmrMoveCount,				  1.,   16.),   "LmrMoveCount", 		0.7),
		OptionTunableParam(SpinType<double>(RazorMultDelta,				  5.,   40.),   "RazorMultDelta", 		0.43),
		OptionTunableParam(SpinType<double>(RfpMultDelta,				  10.,  220.),  "RfpMultDelta", 		0.51),
		OptionTunableParam(SpinType<double>(FutilityMoveCount,			  1.,   16.),   "FutilityMoveCount", 	0.61),
		OptionTunableParam(SpinType<double>(FutilityDelta,				  2.,   50.),   "FutilityDelta", 		0.85),
		OptionTunableParam(SpinType<double>(RazorBaseDelta,				  20.,  500.),  "RazorBaseDelta", 		0.4),
		OptionTunableParam(SpinType<double>(QMaterialDelta,				  700., 1200.), "QMaterialDelta", 		0.45),
		OptionTunableParam(SpinType<double>(QProbeDepth,				  -5.,  1.),    "QProbeDepth", 			1.3),
		OptionTunableParam(SpinType<double>(NNEvalScale,				  7.,   12.),   "NNEvalScale", 			1.5),
		OptionTunableParam(SpinType<double>(ImprovingRate, 				  30.,  90.),   "ImprovingRate" , 	  	0.7),
		OptionTunableParam(SpinType<double>(RfpImprovingSink, 			  1.,   10.),   "RfpImprovingSink",   	1.4),
		OptionTunableParam(SpinType<double>(NullMargin, 				  3.,   50.),   "NullMargin", 			0.3),
		OptionTunableParam(SpinType<double>(NullImprovingSink ,			  1.,   10.), 	"NullImprovingSink", 	1.4),
		OptionTunableParam(SpinType<double>(DynImprovementDepth,		  6.,   16.),   "DynImprovementDepth",  1.6),
		OptionTunableParam(SpinType<double>(TTEvalCorrRate, 			  1.,   3.),    "TTEvalCorrRate", 		2.2),
		OptionTunableParam(SpinType<double>(NullDiffScale, 	 			  800., 1300.), "NullDiffScale", 		0.23),
		OptionTunableParam(SpinType<double>(NullDepth, 	 				  2.,   5.),    "NullDepth", 		    2.7),
		OptionTunableParam(SpinType<double>(NextDepthTimeRed ,			  4.,   8.), 	"NextDepthTimeRed",     2.5),
		OptionTunableParam(SpinType<double>(UnstableMatMargin,			  10.,  80.), 	"UnstableMatMargin",    2.1),
		OptionTunableParam(SpinType<double>(UnstableMultMargin,			  4.,   12.), 	"UnstableMultMargin",   2.7),
		OptionTunableParam(SpinType<double>(MinTimeBranchFactor,		  1.,   2.), 	"MinTimeBranchFactor",  2.),
		OptionTunableParam(SpinType<double>(MaxTimeBranchFactor,		  3.,   5.), 	"MaxTimeBranchFactor",  2.),
		OptionTunableParam(SpinType<double>(ContemptDiv,				  30.,  160.),  "ContemptDiv",          1.8),
		OptionTunableParam(SpinType<double>(ImprovingExtensionRate,		  2., 10.),		"ImprovingExtensionRate", 2.9),
		OptionTunableParam(SpinType<double>(QuietNotPvNodeReduction,      1., 20.),		"QuietNotPvNodeReduction",     2.6),
		OptionTunableParam(SpinType<double>(QuietCutNodeReduction,        1., 20.),		"QuietCutNodeReduction",       2.6),
		OptionTunableParam(SpinType<double>(QuietCheckReduction,		  5., 60.),		"QuietCheckReduction",		   2.1),
		OptionTunableParam(SpinType<double>(QuietExtensionReduction,	  5., 50.),		"QuietExtensionReduction",     2.2),
		OptionTunableParam(SpinType<double>(QuietPawnMoveReduction,		  1., 15.),		"QuietPawnMoveReduction",      2.7),
		OptionTunableParam(SpinType<double>(QuietImprovingReductionRate,  3., 15.),		"QuietImprovingReductionRate", 2.6),
		OptionTunableParam(SpinType<double>(QuietHashCapReduction,		  10.,50.),		"QuietHashCapReduction",	   2.1),
		OptionTunableParam(SpinType<double>(QuietKillerMoveReduction,	  5., 40.),		"QuietKillerMoveReduction",    1.9),
		OptionTunableParam(SpinType<double>(QuietTotalReductionRate,	  25.,80.),		"QuietTotalReductionRate",     1.7),
		OptionTunableParam(SpinType<double>(CaptureNotPvNodeReduction,    1., 20.),		"CaptureNotPvNodeReduction",   2.6),
		OptionTunableParam(SpinType<double>(CaptureCutNodeReduction,      1., 20.),		"CaptureCutNodeReduction",     2.6),
		OptionTunableParam(SpinType<double>(CaptureCheckReduction,        5., 60.),		"CaptureCheckReduction",       2.1),
		OptionTunableParam(SpinType<double>(CaptureHashCapReduction,      10.,50.),		"CaptureHashCapReduction",     2.1),
		OptionTunableParam(SpinType<double>(CaptureKillerMoveReduction,   5., 50.),		"CaptureKillerMoveReduction",  2.),
		OptionTunableParam(SpinType<double>(CaptureExtensionReduction,    5., 50.),		"CaptureExtensionReduction",	2.2),
		OptionTunableParam(SpinType<double>(CaptureImprovingReductionRate,3., 20.),		"CaptureImprovingReductionRate",2.4),
		OptionTunableParam(SpinType<double>(CaptureTotalReductionRate,    20.,80.),		"CaptureTotalReductionRate",    1.6),
		OptionTunableParam(SpinType<double>(HalfMovesEvalLimit,			  5., 40.),		"HalfMovesEvalLimit",			1.1),
		OptionTunableParam(SpinType<double>(NullVerifyDepth,			  2., 10.),		"NullVerifyDepth",			    2.2),
		OptionTunableParam(SpinType<double>(MoveCheckExtensionRate,		  8., 16.),		"MoveCheckExtensionRate",		2.5),
		OptionTunableParam(SpinType<double>(MoveCheckExtensionDiv,		  8., 16.),		"MoveCheckExtensionDiv",		2.5),
		OptionTunableParam(SpinType<double>(ImprovingExtensionMateRate,   4., 16.),		"ImprovingExtensionMateRate",   2.2),
		OptionTunableParam(SpinType<double>(MateThreadFracExtensionRate,  10., 30.),	"MateThreadFracExtensionRate",  1.9),
		OptionTunableParam(SpinType<double>(MateThreadFracExtensionDiv,   10., 18.),	"MateThreadFracExtensionDiv",   2.3),
		OptionTunableParam(SpinType<double>(MaxMoveExtensionRate,		  10., 30.),	"MaxMoveExtensionRate",		 1.9),
		OptionTunableParam(SpinType<double>(MaxMoveExtensionDiv,		  10., 18.),	"MaxMoveExtensionDiv",		 2.3),
		OptionTunableParam(SpinType<double>(NullVerifyDepthMult,		  1.,  12.),	"NullVerifyDepthMult",  	 2.8),
		OptionTunableParam(SpinType<double>(ExtensionDepth,				  4.,  32.),    "ExtensionDepth",			 1.2),
		OptionTunableParam(SpinType<double>(SingularDepth, 				  2.,  8.), 	"SingularDepth", 			 2.9),
		OptionTunableParam(SpinType<double>(SingularDepthMargin, 		  1.,  4.), 	"SingularDepthMargin",  	 2.4),
		OptionTunableParam(SpinType<double>(SingularExtensionRate, 		  1.,  24.), 	"SingularExtensionRate", 	 2.4),
		OptionTunableParam(SpinType<double>(SingularBetaDepthMult, 		  1.,  6.), 	"SingularBetaDepthMult",     2.1),
		OptionTunableParam(SpinType<double>(SingularDepthMult, 	   		  90., 180.), 	"SingularDepthMult", 		 1.4),
		OptionTunableParam(SpinType<double>(SingularDepthBase, 	   		  400., 650.), 	"SingularDepthBase", 		 0.7),
		OptionTunableParam(SpinType<double>(SingularBetaExtensionRate, 	  1.,   12.), 	"SingularBetaExtensionRate", 2.2),
		OptionTunableParam(SpinType<double>(QuietMoveScoreReductionRate,  1., 25.),		"QuietMoveScoreReductionRate",  0.25),
		OptionTunableParam(SpinType<double>(QuietMoveScoreReductionDiv,   1., 15.),		"QuietMoveScoreReductionDiv",   0.34),
		OptionTunableParam(SpinType<double>(CaptureMoveScoreReductionDiv, 30.,120.),	"CaptureMoveScoreReductionDiv", 1.2),
		OptionTunableParam(SpinType<double>(KnightCapturedScore,		  260., 350.),  "KnightCapturedScore",  0.8),
		OptionTunableParam(SpinType<double>(BishopCapturedScore,		  260., 350.),  "BishopCapturedScore",  0.8),
		OptionTunableParam(SpinType<double>(ToKnightPromoScore,			  80.,  300.),  "ToKnightPromoScore",   1.3),
		OptionTunableParam(SpinType<double>(ToBishopPromoScore,			  50.,  300.),  "ToBishopPromoScore",   1.3),
		OptionTunableParam(SpinType<double>(ToRookPromoScore,			  150., 500.),  "ToRookPromoScore",     1.3),
		OptionTunableParam(SpinType<double>(ToQueenPromoScore,			  700., 1020.), "ToQueenPromoScore",    1.3),
		}, 
};

SearchLimits UniversalChessInterface::loadSearchLimits(std::istringstream& strm, std::string token) {
	SearchLimits limits;
	limits.depth = MaxDepth;
    limits.nodes = limits.qnodes = 0;

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
void UniversalChessInterface::loop(int argc, const char* argv[]) {
	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	if (argc > 1 and std::string(argv[1]) == "--self-play")
		parseSelfPlay();

#if defined(_ENABLE_TUNING)
	GlobParamMapping.createMapping();
#endif

	std::cout << "Polish Chess Engine, " << EngineName << " by " << Author << '\n';

	std::string command;
	int arg_it = 1;

	do {
		if (arg_it < argc)
			command = argv[arg_it++];

		else if (!std::getline(std::cin, command))
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
		else if (token == "bench")			parseBench(strm);

#if defined(_UCI_DEBUG_UTILS)
		else if (token == "see")			parseSEE(strm);
		else if (token == "nneval")			parseNNEval(strm);
#endif

	} while (command != "quit");
}

std::vector<OptionTunableParam>& UniversalChessInterface::getTunableOptions() {
	return _options.tunable_params_opt;
}

void UniversalChessInterface::parseUCI() {
	std::cout << "id name " << EngineName << '\n'
			  << "id author " << Author << '\n'
			  << "uciok" << '\n';
}

void UniversalChessInterface::parseNewGame() {
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

			if (move.isNull() or move.getPieceColor(_pos) != _pos.getTurn()) {
				std::cout << "Invalid move" << std::endl;
				_game.clear();
				break;
			}
			else if (_game.getMoveCount() >= MaxGameMoves - MaxSelDepth) {
				std::cout << "Game buffer overflow" << std::endl;
				_game.clear();
				break;
			}

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
	_search.findBestMove(_pos, _game, limits);
}

void UniversalChessInterface::parseIsReady() {
	std::cout << "readyok" << std::endl;
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

			_options.hash_opt.set(val);
			_search.resizeHashTT(_options.hash_opt.getCurrentValue() * 1_MB);
		}
	}

	if (option == "SyzygyPath") {
		strm >> std::skipws >> token;
		
		if (token == "value") {
			strm >> std::skipws >> token;

			if (token.find(';') != std::string::npos) {
				std::cout << "Multiple paths not supported" << std::endl;
				return;
			}

			_options.syzygy_opt.set(token);
			SyzygyTablebase::get().loadSyzygyFile(token);
		}
	}

#if defined(_ENABLE_TUNING)

	for (OptionTunableParam& param : _options.tunable_params_opt) {
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

void UniversalChessInterface::parseBench(std::istringstream& strm) {
	static constexpr int BenchDepth = 5;

	int depth = BenchDepth;
	strm >> std::skipws >> depth;

	if (depth <= 0 or depth >= MaxDepth) 
		depth = BenchDepth;

	Timer timer;
	size_t total_nodes = 0;

	timer.go();

	std::for_each(Utils::BenchmarkSet.begin(), Utils::BenchmarkSet.end(), 
		[&](const std::string_view& fen) {
			Position pos(fen);

			SearchLimits limits;
			limits.depth = depth;
    		limits.nodes = 0; // no node limit
    		limits.wtime = limits.btime = 0;
    		limits.winc  = limits.binc =  0;

			FullInfoRecord tmpgame;
			SearchResults results;

#if defined(DEBUG)
			std::cout << "Searching " << fen << "..." << std::endl;
#endif

			const Move32b bm = _search.findBestMove<Search::SEARCH_NO_INFO>(pos, tmpgame, limits, results);
			_declUnused(bm);

			total_nodes += results.nodes_cnt;
		});

	const time_ms_t total_time_ms = timer.duration();

	std::cout << "Searched " << total_nodes << " nodes in " << (total_time_ms / 1000.) << 's' << std::endl;
}

void UniversalChessInterface::parseShowOptions() {
	_options.hash_opt.print();
	_options.clear_hash_opt.print();
	_options.syzygy_opt.print();

#if defined(_ENABLE_TUNING)
	for (OptionTunableParam& param : _options.tunable_params_opt) {
		param.print();
	}
#endif
}

void UniversalChessInterface::parseSelfPlay() {
#ifdef _MSC_VER
	if (_setmode(_fileno(stdout), _O_BINARY) == -1 or
		_setmode(_fileno(stdin), _O_BINARY) == -1) {
		std::cout << "Failed to set binary IO" << std::endl;
		return;
	}

	std::cout << "Toggled binary IO" << std::endl;
#endif
}
