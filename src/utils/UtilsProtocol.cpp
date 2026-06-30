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

#include "UtilsProtocol.hpp"
#include "TrainEntry.hpp"
#include "StaticEval.hpp"
#include "Tuning.hpp"
#include <variant>
#include <execution>

namespace Utils
{

void UtilsProtocol::parseSelfPlay(TournamentCollector& collector, 
                                  std::istringstream& strm) {
    std::string token;
    strm >> std::skipws >> token;

    const int games_count = std::stoi(token);
    if (games_count <= 0) {
        std::cout << "Invalid games count" << std::endl;
        return;
    }

    strm >> std::skipws >> token;

    const int thread_cnt = std::stoi(token);
    if (thread_cnt <= 0) {
        std::cout << "Invalid thread number" << std::endl;
        return;
    }

    strm >> std::skipws >> token;
    const std::filesystem::path log_dir = token;

    strm >> std::skipws >> token;
    const std::filesystem::path err_log_dir = token;

    strm >> std::skipws >> token;
    SearchLimits limits = UniversalChessInterface::loadSearchLimits(strm, token);

    const TournamentCollector::TournamentPacket packet = {
        static_cast<size_t>(games_count),
        static_cast<uint>(thread_cnt),
        log_dir,
        err_log_dir,
        limits,
    };

    collector.startTournament(packet);
}

void UtilsProtocol::parseShowPositions(std::istringstream& strm) {
    std::string filepath;
    strm >> std::skipws >> filepath;

    std::ifstream input(filepath, std::ios::ios_base::binary);

    if (!input) {
        std::cout << "Failed to open file: " << filepath << std::endl;
        return;
    }

    int begin, count;

    strm >> std::skipws >> begin >> std::skipws >> count;

    std::vector<ExtPackedPosition> pack_positions = ExtPackedPosition::fullRead(input);
    size_t n = pack_positions.size();

    if (begin < 0 or begin + count > n) {
        std::cout << "Invalid access indexes" << std::endl;
        return;
    }

    for (size_t i = begin; i < begin + count; i++) {
        Position unpack = ExtPackedPosition::unpacked(pack_positions[i]);
        unpack.print();
    }

    std::cout.flush();
}

bool verifyTrainData(std::ifstream& input, 
                     size_t& verified_cnt, 
                     TrainingDataEntry::Result8b expected_result) 
{
    ASSERTNOLOG(input.is_open());

    TrainingDataEntry entry;
    verified_cnt = 0;

    while (TrainingDataEntry::read(input, entry)) {
        const PackedPosition pack = entry.getPosition();
        const Score white_score = entry.getWhiteScore();
        const TrainingDataEntry::Result8b game_result = entry.getGameResult();

        if (const Position pos = PackedPosition::unpacked(pack); 
            game_result != expected_result or 
            !TournamentCollector::explicitFilterPolicy(PackedPosition::unpacked(pack), white_score)) {
            
            std::cout << "Verification failed, invalid packed position\n";

            if (pos.isValid()) pos.print();
            else               pack.print();

            std::cout << std::flush;
            return false;
        }

        verified_cnt++;
    }

    if (getIStreamBytesLeft(input) > 0) {
        std::cout << "Verification failed, cannot read entire file" << std::endl;
        return false;
    }
    
    std::cout << "Verified " << verified_cnt << " positions" << std::endl;
    return true;
}

void UtilsProtocol::parseVerifySession(std::istringstream& strm) {
    std::vector<std::filesystem::path> dirs;
    std::string curr_dir;

    while (strm >> std::skipws >> curr_dir) {
        dirs.emplace_back(curr_dir);
    }

    size_t total_verified_positions = 0;

    for (const auto& tournament_dir : dirs) {
        for (uint session = 1; session <= SelfPlaySessionCountLimit; session++) {
            std::filesystem::path session_fp = "session" + std::to_string(session);

            if (!std::filesystem::exists(tournament_dir / session_fp) or session_fp.empty())
                continue;

            for (uint id = 1; id <= static_cast<uint>(PlatformThreadLimit); id++) {
                {
                    std::filesystem::path fp = tournament_dir / session_fp / getWhiteWinOutputFile(id);
                    std::ifstream input(fp, std::ios_base::binary);

                    if (input) {
                        std::cout << "Verificating " << fp << "..." << std::endl;
                        
                        size_t verified_cnt = 0;

                        if (!verifyTrainData(input, verified_cnt, TrainingDataEntry::WHITE_WIN)) {
                            std::cout << "Verification failed on file: " << fp << std::endl;
                            return;
                        }

                        total_verified_positions += verified_cnt;
                    }
                }

                {
                    std::filesystem::path fp = tournament_dir / session_fp / getBlackWinOutputFile(id);
                    std::ifstream input(fp, std::ios_base::binary);

                    if (input) {
                        std::cout << "Verificating " << fp << "..." << std::endl;
                        
                        size_t verified_cnt = 0;

                        if (!verifyTrainData(input, verified_cnt, TrainingDataEntry::BLACK_WIN)) {
                            std::cout << "Verification failed on file: " << fp << std::endl;
                            return;
                        }

                        total_verified_positions += verified_cnt;
                    }
                }

                {
                    std::filesystem::path fp = tournament_dir / session_fp / getDrawOutputFile(id);
                    std::ifstream input(fp, std::ios_base::binary);

                    if (input) {
                        std::cout << "Verificating " << fp << "..." << std::endl;
                        
                        size_t verified_cnt = 0;

                        if (!verifyTrainData(input, verified_cnt, TrainingDataEntry::DRAW)) {
                            std::cout << "Verification failed on file: " << fp << std::endl;
                            return;
                        }

                        total_verified_positions += verified_cnt;
                    }
                }
            }
        }
    }

    std::cout << "Verification succeded" << std::endl;
    std::cout << "Total of " << total_verified_positions << " positions searched" << std::endl;
}

void UtilsProtocol::parseSPSA(std::istringstream& strm) {
    int thread_count;
    strm >> std::skipws >> thread_count;

    std::string spsa_log_path;
    strm >> std::skipws >> spsa_log_path;

    _tuner.start(thread_count, spsa_log_path);
}

void UtilsProtocol::parsePerft(std::istringstream& strm) {
#if defined(DEBUG)
    static constexpr int DepthTestLimit = 4;
#else
    static constexpr int DepthTestLimit = 6;
#endif

    std::string token;
    strm >> std::skipws >> token;

    using variant_policy_t = std::variant<
        std::execution::sequenced_policy,
        std::execution::parallel_policy
    >;
    variant_policy_t var_policy;

    if (token == "-p") {
        var_policy = std::execution::par;
    }
    else {
        std::cout << "Unknown option for test_perft" << std::endl;
        var_policy = std::execution::seq;
    }

    static const auto perft_on_set = [](const auto policy) {
        std::atomic<bool> status = true;
        std::atomic<time_ms_t> total_duration_ms = 0_ms;

        std::for_each(policy, PerftStandard.begin(), PerftStandard.end(), 
            [&status, &total_duration_ms](std::string_view test) {
                std::istringstream ss(static_cast<std::string>(test));
                
                std::string fen;
                std::string token;

                for (int i = 0; i < 6; i++) {
                    ss >> std::skipws >> token;
                    fen += ' ' + token;
                }

                Position pos(fen);

                while (ss >> std::skipws >> token) {
                    const auto depth = std::stoi(token.substr(1));
                    
                    if (depth > DepthTestLimit) break;

                    ss >> std::skipws >> token;
                    const auto nodes = std::stoull(token);

                    time_ms_t duration_ms;
                    const auto perft_nodes = pos.goPerft(depth, duration_ms);

                    total_duration_ms.fetch_add(duration_ms);

                    if (nodes != perft_nodes) {
                        std::cout << "Invalid node count for fen: " << fen << std::endl;
                        std::cout << "Got " << perft_nodes << ", but target is " << nodes << ' '
                                  << "at depth " << depth << std::endl;
                        status = false;
                        break;
                    }

                    std::cout << std::flush;
                }
            }
        );

        const float total_sec_duration = total_duration_ms.load() / 1000.f;

        std::cout << "Perft suit test " << (status.load() ? "passed" : "failed") << " in " 
                  << total_sec_duration << " seconds" << std::endl;

    }; // perft_on_set

    std::visit(perft_on_set, var_policy);
}

void UtilsProtocol::loop(int argc, const char* argv[]) {
    std::ios_base::sync_with_stdio(false);

#if defined(_ENABLE_TUNING)
    GlobParamMapping.createMapping();
#endif

    if (argc > 1 and std::string(argv[1]) == "--self-play")
        UniversalChessInterface::parseSelfPlay();

    ProcExecArg = argv[0];

    std::cout << "Utility build of Leaf" << '\n';

    std::string command;

    do {
        if (!std::getline(std::cin, command))
            command = "quit";

        std::istringstream strm(command);
        std::string token;

        strm >> std::skipws >> token;

             if (token == "uci")                   parseUCI();
        else if (token == "ucinewgame")            parseNewGame();
        else if (token == "position")               parsePosition(strm);
        else if (token == "print")                 _pos.print();
        else if (token == "go")                    parseGo(strm);
        else if (token == "isready")               parseIsReady();
        else if (token == "export_net")            parseNeuralNet(strm);
        else if (token == "rewrite_header")        parseRewriteNet(strm);
        else if (token == "options")               parseShowOptions();
        else if (token == "setoption")             parseSetOptions(strm);
        else if (token == "test_pack")             packedPositionTests();
        else if (token == "test_ccr_one_hour")     ccrOneHourTest();
        else if (token == "test_null_move")        nullMoveTest();
        else if (token == "test_see")              seeTests();
        else if (token == "test_pack_on")          parsePackedFile(strm);
        else if (token == "test_extpack_on")       parseExtPackedFile(strm);
        else if (token == "test_perft")            parsePerft(strm);
        else if (token == "self_play")             parseSelfPlay(_collector, strm);
        else if (token == "load_openings")         GlobOpeningGenerator.load();
        else if (token == "view_positions")        parseShowPositions(strm);
        else if (token == "verify_session")        parseVerifySession(strm);
        else if (token == "spsa")                  parseSPSA(strm);

#if defined(_UCI_DEBUG_UTILS)
        else if (token == "see")                   parseSEE(strm);
        else if (token == "nneval")                parseNNEval(strm);
#endif

    } while (command != "quit");
}

} // namespace Utils
