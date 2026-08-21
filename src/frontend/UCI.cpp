/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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

_INLINE bool isValidNumber(const std::string& str) {
    return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

_INLINE bool isSigned(const std::string& str) {
    return !str.empty() and str[0] == '-';
}

_INLINE bool isValidUnsigned(const std::string& str) {
    return !isSigned(str) and isValidNumber(str);
}

opt::Options UniversalChessInterface::_options;

engine::SearchLimits UniversalChessInterface::loadSearchLimits(std::istringstream& strm, std::string token) {
    engine::SearchLimits limits;
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
    : _search(tt::TranspositionTable(tt::DefaultTTSizeMb))
    , _pos(StartposFEN)
{}

void UniversalChessInterface::init() {
#if defined(_WIN32)
    try {
        mem::enableLargePagesPrivilegeWin32();
    }
    catch (const std::runtime_error& e) {
        std::cout << "Failed to setup LargePages Privileges: " << e.what() << std::endl;
    }
#endif
    SlidersAttacks::initTables();
}

void UniversalChessInterface::finish() {}

void UniversalChessInterface::loop(int argc, const char* argv[]) {
    // C-style streams aren't used there
    std::ios_base::sync_with_stdio(false);

    if (argc > 1 and std::string(argv[1]) == "--self-play")
        parseSelfPlay();

    std::cout << "Polish Chess Engine, " << EngineName << " by " << EngineAuthor << '\n';

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

        // Standard UCI commands
             if (token == "uci")            parseUCI();
        else if (token == "ucinewgame")     parseNewGame();
        else if (token == "position")       parsePosition(strm);
        else if (token == "go")             parseGo(strm);
        else if (token == "isready")        parseIsReady();
        else if (token == "options")        parseShowOptions();
        else if (token == "setoption")      parseSetOptions(strm);

        // Utility commands
        else if (token == "export_net")     parseNeuralNet(strm);
        else if (token == "rewrite_header") parseRewriteNet(strm);
        else if (token == "print")          _pos.print();
        else if (token == "bench")          parseBench(strm);

        // Debug commands
#if defined(_UCI_DEBUG_UTILS)
        else if (token == "see")            parseSEE(strm);
        else if (token == "nneval")         parseNNEval(strm);
#endif

    } while (command != "quit");
}

std::vector<opt::OptionTunableParam>& UniversalChessInterface::getTunableOptions() {
    return _options.tunable_params_opt;
}

void UniversalChessInterface::parseUCI() {
    std::cout << "id name " << EngineName << '\n'
              << "id author " << EngineAuthor << '\n'
              << "uciok" << '\n';
}

void UniversalChessInterface::parseNewGame() {
    _game.clear();
    _search.onNewGame();
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

            if (move.isNullMove() or move.getPieceColor(_pos) != _pos.getTurn()) {
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
            ASSERT_NO_LOG(_pos.make(move));
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
            _pos.goPerft(depth);
        }

        return;
    }
    
    engine::SearchLimits limits = loadSearchLimits(strm, token);
    _declUnused(_search.findBestMove(_pos, _game, limits));
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
    int score = _pos.staticExchangeEval<false>(org, dst, _pos.pieceOn(dst, _pos.getOppositeTurn()), 
                                               _pos.pieceOn(org, _pos.getTurn()));
    std::cout << score << std::endl;
}

void UniversalChessInterface::parseNNEval(std::istringstream& strm) {
    std::string fen;
    std::getline(strm >> std::ws, fen);

    Position pos;

    if (fen == "startpos")
        pos.setStartingPos();
	else if (fen == "kiwipete")
        pos.setByFEN(std::string(KiwipeteFEN));
    else
        pos.setByFEN(fen);

    const sc::Score score = nn::NEval::evaluate(nn::GlobPackedNetwork, pos);

    std::cout << static_cast<int>(score) << std::endl;
}

#endif

bool UniversalChessInterface::parseNeuralNet(std::istringstream& strm) {
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
    
    return status;
}

void UniversalChessInterface::parseRewriteNet(std::istringstream& strm) {
    std::string in_path, out_path;
    strm >> std::skipws >> in_path >> std::skipws >> out_path;

    nn::PackedNeuralNetwork::Header header{};
    // modify it manually
    header.dual_hl = nn::NetworkDualHiddenLayer;
    header.layer_size[0] = nn::NetworkInputSize;
    header.layer_size[1] = nn::NetworkHiddenLayerSize;
    header.layer_size[2] = nn::NetworkOutputSize;
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
            _search.clearHash();
        }
    }
    
    if (option == "Hash") {
        strm >> std::skipws >> token;

        if (token == "value") {
            strm >> std::skipws >> token;
            
            ll val = std::stoi(token);

            try {
                _options.hash_opt.set(val);
            } 
            catch (const std::runtime_error&) {
                val = std::clamp(val, 
                                 _options.hash_opt.value.min_value, 
                                 _options.hash_opt.value.max_value);

                _options.hash_opt.set(val);
            }

            _search.resizeHash(_options.hash_opt.getCurrentValue() * 1_MB);
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
            _UNUSED const bool status = SyzygyTablebase::get().loadSyzygyFile(token);

            if (!status) {
                std::cout << "Failed to load Syzygy Tablebase" << std::endl;
                return;
            }
            else {
                std::cout << "Loaded Syzygy Tablebase" << std::endl;
                return;
            }
        }
    }

    if (option == "NeuralNetPath") {
        strm >> std::skipws >> token;
        
        if (token == "value") {
            strm >> std::skipws >> token;

            if (token.find(';') != std::string::npos) {
                std::cout << "Multiple paths not supported" << std::endl;
                return;
            }

            std::istringstream ss(token);

            if (parseNeuralNet(ss)) {
                _options.neural_net_opt.set(token);
            }
        }
    }

#if defined(_ENABLE_TUNING)
    for (opt::OptionTunableParam& param : _options.tunable_params_opt) {
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
    static constexpr int BenchDepth = 18;

    int depth = BenchDepth;
    strm >> std::skipws >> depth;

    if (depth <= 0 or depth >= MaxDepth) 
        depth = BenchDepth;

    clk::Timer timer;
    size_t total_nodes = 0;

    timer.go();

    std::for_each(utils::BenchmarkSet.begin(), utils::BenchmarkSet.end(), 
        [&](const std::string_view& fen) {
            Position pos(fen);

            engine::SearchLimits limits;
            limits.depth = depth;
            limits.nodes = 0; // no node limit
            limits.wtime = limits.btime = 0;
            limits.winc  = limits.binc =  0;

            FullInfoRecord tmpgame;
            engine::SearchResults results;

#if defined(DEBUG)
            std::cout << "Searching " << fen << "..." << std::endl;
#endif

            const Move32b bm = _search.findBestMove<engine::SEARCH_NO_INFO>(pos, tmpgame, limits, results);
            _declUnused(bm);

            total_nodes += results.nodes_cnt;
        });

    const clk::milliseconds total_time_ms = timer.getDurationMs();

    std::cout << "Searched " << total_nodes << " nodes in " << (total_time_ms / 1000.) << 's' << std::endl;
}

void UniversalChessInterface::parseShowOptions() {
    auto& [hash_opt, 
           clear_hash_opt, 
           syzygy_opt, 
           neural_net_opt, 
           tunable_params_opt] = _options;

    std::apply([](const auto&... opt) { ((opt.print()), ...); }, 
        std::tie(hash_opt, clear_hash_opt, syzygy_opt, neural_net_opt));

#if defined(_ENABLE_TUNING)
    for (opt::OptionTunableParam& param : _options.tunable_params_opt) {
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
