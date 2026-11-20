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
#include <map>
#include <iomanip>

namespace utils
{

void UtilsProtocol::parseSelfPlay(TournamentCollector& collector, 
                                  std::istringstream& strm) 
{
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
    search::utils::SearchLimits limits = UniversalChessInterface::loadSearchLimits(strm, token);

    const TournamentCollector::TournamentPacket packet = {
        static_cast<size_t>(games_count),
        static_cast<uint>(thread_cnt),
        log_dir,
        limits,
    };

    std::cout << packet.selfplay_filename << std::endl;
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
    ASSERT_NO_LOG(input.is_open());

    verified_cnt = 0;

    for (TrainingDataEntry entry; TrainingDataEntry::read(input, entry); ) {
        const PackedPosition pack = entry.getPosition();
        const sc::Score white_score = entry.getWhiteScore();
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

    for (std::string curr_dir; strm >> std::skipws >> curr_dir; ) {
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
                    std::filesystem::path fp = tournament_dir / session_fp 
                                                / paths::PathsManager.getWhiteWinOutputFileName(id);
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
                    std::filesystem::path fp = tournament_dir / session_fp 
                                                / paths::PathsManager.getBlackWinOutputFileName(id);
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
                    std::filesystem::path fp = tournament_dir / session_fp 
                                                / paths::PathsManager.getDrawOutputFileName(id);
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

void UtilsProtocol::parseBulletFormat(std::istringstream& strm) {
    std::filesystem::path selfplay_root;
    strm >> std::skipws >> selfplay_root;

    if (!std::filesystem::exists(selfplay_root)) {
        std::cerr << "Invalid selfplay directory root: " << selfplay_root << '\n';
        return;
    }

    std::filesystem::path bf_selfplay_root = selfplay_root;
    bf_selfplay_root.replace_filename(selfplay_root.filename().string() + "_serialized");

    std::vector<std::filesystem::path> tdf_files;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(selfplay_root)) {
        if (entry.is_regular_file() and entry.path().extension() == ".tdf") {
            tdf_files.push_back(entry.path());
        }
    }

    std::atomic<bool> success = true;

    std::for_each(std::execution::par, tdf_files.begin(), tdf_files.end(), [&](const auto& fp) {
        {
            static std::mutex m;
            std::lock_guard lock(m);
            std::cout << "Processing " << fp << std::endl;
        }

        const std::filesystem::path rfp = std::filesystem::relative(fp, selfplay_root);
        const std::filesystem::path target = bf_selfplay_root / rfp;

        {
            static std::mutex m;
            std::lock_guard lock(m);
            std::filesystem::create_directories(target.parent_path());
        }

        std::ifstream input(fp, std::ios_base::binary);

        if (!input) {
            std::cout << "Failed to open input file: " << fp << std::endl;
            return;
        }

        std::ofstream output(target);

        if (!output) {
            std::cout << "Failed to open output file: " << target << std::endl;
            return;
        }

        for (TrainingDataEntry entry; 
             TrainingDataEntry::read(input, entry) and success.load(); ) 
        {
            const BulletChessBoard bullet_board = TrainingDataEntry::toBulletFormat(entry);

            if (!BulletChessBoard::write(output, bullet_board)) {
                std::cout << "Failed to write bullet board to output: " << target << std::endl;
                success.store(false);
                break;
            }
        }

        if (!success.load())
            std::cout << "Aborting from thread" << std::endl;
    });

    if (success.load()) 
        std::cout << "Successfully serialized files to bullet format" << std::endl;
    else 
        std::cout << "Failed to serialize files to bullet format" << std::endl;
}

void UtilsProtocol::parseTestBulletFormat(std::istringstream& strm) {
    std::filesystem::path fp_bf;
    strm >> std::skipws >> fp_bf;

    std::ifstream input_bf(fp_bf);

    if (!input_bf) {
        std::cout << "Failed to open a file: " << fp_bf << std::endl;
        return;
    }

    std::filesystem::path fen_fp;
    strm >> std::skipws >> fen_fp;

    std::ifstream input_fen(fen_fp);

    if (!input_fen) {
        std::cout << "Failed to open a file: " << fen_fp << std::endl;
        return;
    }

    std::string fen;
    BulletChessBoard bf_entry_valid;

    static constexpr sc::Score FixedScore = 120;

    bool success = true;

    while (BulletChessBoard::read(input_bf, bf_entry_valid) and std::getline(input_fen, fen)) {
        const ExtPackedPosition packed = ExtPackedPosition::packed(Position(fen));
        const TrainingDataEntry entry(packed, FixedScore, TrainingDataEntry::BLACK_WIN);
        const BulletChessBoard bf_entry = TrainingDataEntry::toBulletFormat(entry);

        if (bf_entry != bf_entry_valid) {
            std::cout << "Bullet test failed" << std::endl;
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "Bullet test passed successfully" << std::endl;
    }
    else {
        std::cout << "Bullet test failed: aborting" << std::endl;
    }
}

namespace {
/* 'shuffle_data' only permutes records that already are in bullet format.
*  Raw Leaf selfplay entries share the very same 32-byte layout, so pointing
*  this command at un-serialized training data silently yields a file that
*  bullet decodes with different nibble semantics and side-to-move orientation.
*  Detect that case instead of quietly corrupting the dataset.
*/
bool looksLikeBulletFormat(const BulletChessBoard& bf) {
    static constexpr uint8_t OurKingNibble = 0x5, TheirKingNibble = 0xD;

    const size_t piece_cnt = bf.occ.popCount();

    if (piece_cnt > 32) // nibbles are 4-bit packed, at most 32 pieces fit
        return false;

    uint8_t our_kings = 0, their_kings = 0;

    for (size_t i = 0; i < piece_cnt; i++) {
        const uint8_t nibble = (bf.pcs[i / 2] >> (4 * (i % 2))) & 0x0F;

        if ((nibble & 0x07) > 5) // bullet knows only 6 piece types
            return false;

        our_kings   += nibble == OurKingNibble;
        their_kings += nibble == TheirKingNibble;
    }

    return our_kings == 1 and their_kings == 1;
}
} // namespace

void UtilsProtocol::parseDataShuffles(std::istringstream& strm) {
    std::filesystem::path selfplay_root;
    strm >> std::skipws >> selfplay_root;

    if (!std::filesystem::exists(selfplay_root)) {
        std::cerr << "Invalid selfplay directory root: " << selfplay_root << '\n';
        return;
    }

    std::filesystem::path bf_selfplay_root = selfplay_root;
    bf_selfplay_root.replace_filename(selfplay_root.filename().string() + "_shuffled");

    std::vector<std::filesystem::path> selfplay_dirs;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(selfplay_root)) {
        if (entry.path().filename().string().find("selfplay") == 0 and entry.is_directory()) {
            selfplay_dirs.push_back(entry.path());
        }
    }

    std::for_each(std::execution::par, selfplay_dirs.begin(), selfplay_dirs.end(), [&](const auto& selfplay_fp) {
        {
            static std::mutex m;
            std::lock_guard lock(m);
			std::cout << "Processing " << selfplay_fp << std::endl;
        }

        const std::filesystem::path rfp = std::filesystem::relative(selfplay_fp, selfplay_root);
        const std::filesystem::path target = (bf_selfplay_root / rfp).replace_extension(".tdf");

        for (const auto& entry : std::filesystem::recursive_directory_iterator(selfplay_fp)) {
            if (!entry.is_regular_file() or entry.path().extension() != ".tdf")
                continue;

            std::ifstream probe(entry.path(), std::ios_base::binary);
            BulletChessBoard first_entry;

            if (BulletChessBoard::read(probe, first_entry) and !looksLikeBulletFormat(first_entry)) {
                static std::mutex m;
                std::lock_guard lock(m);
                std::cout << "Input " << entry.path() << " is not bullet-format data, aborting. "
                          << "Run 'to_bullet_format " << selfplay_root.string() 
                          << "' first, then shuffle its output." << std::endl;
                return;
            }

            break;
        }

        {
            static std::mutex m;
            std::lock_guard lock(m);
            std::filesystem::create_directories(target.parent_path());
        }

        std::ofstream output(target, std::ios_base::binary);

        if (!output) {
            std::cout << "Failed to open output file: " << target << std::endl;
            return;
        }

        std::vector<BulletChessBoard> entries;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(selfplay_fp)) {
            if (entry.is_regular_file() and entry.path().extension() == ".tdf") {
                std::ifstream input(entry.path(), std::ios_base::binary);

                if (!input) {
                    std::cout << "Failed to open input file: " << entry.path() << std::endl;
                    return;
                }

                for (BulletChessBoard entry; BulletChessBoard::read(input, entry); ) {
                    entries.push_back(entry);
                }
            }
        }

        std::shuffle(entries.begin(), entries.end(), rnd::GlobMersenne);

        for (const auto& entry : entries) {
            if (!BulletChessBoard::write(output, entry)) {
                std::cout << "Failed to write entry to output: " << target << std::endl;
                return;
            }
		}
    });
}

namespace {

/* Training records are 32 bytes in both formats:
*  [0, 24) position, [24, 26) score, 26 result, 27 stm king, 28 opponent king.
*  Byte 29 is the raw 'relative' flag (padding once serialized) and the last two
*  bytes are padding in both, so they are excluded - otherwise two copies of the
*  same sample with different garbage there would be counted as distinct.
*/
constexpr size_t HashedRecordBytes = 29;

uint64_t hashTrainingRecord(const std::array<uint8_t, 32>& record) {
    uint64_t hash = 0xCBF29CE484222325ull;

    for (size_t i = 0; i < HashedRecordBytes; i++) {
        hash ^= record[i];
        hash *= 0x00000100000001B3ull;
    }

    return rnd::mixSeed(hash);
}

void sortUnique(std::vector<uint64_t>& hashes) {
    std::sort(hashes.begin(), hashes.end());
    hashes.erase(std::unique(hashes.begin(), hashes.end()), hashes.end());
}

std::string abbreviatePath(std::string path, size_t max_len) {
    if (path.size() <= max_len)
        return path;

    return "..." + path.substr(path.size() - (max_len - 3));
}

} // namespace

/* Reports how much of a generated dataset is actually distinct.
*  Selfplay that replays the same games looks perfectly valid to every format
*  checker, so this is the check that needs to run before a training run.
*/
void UtilsProtocol::parseDataCheck(std::istringstream& strm) {
    std::filesystem::path root;
    strm >> std::skipws >> root;

    if (root.empty()) {
        std::cout << "Usage: data_check <training data root>" << std::endl;
        return;
    }

    if (!std::filesystem::exists(root)) {
        std::cout << "Invalid training data root: " << root << std::endl;
        return;
    }

    std::map<std::string, std::vector<std::filesystem::path>> dir_files;
    size_t file_cnt = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() and entry.path().extension() == ".tdf") {
            dir_files[entry.path().parent_path().string()].push_back(entry.path());
            file_cnt++;
        }
    }

    if (!file_cnt) {
        std::cout << "No '.tdf' files found under " << root << std::endl;
        return;
    }

    std::cout << "Checking " << file_cnt << " file(s) under " << root << std::endl;
    std::cout << std::left << std::setw(60) << "directory" << std::right
              << std::setw(14) << "records"
              << std::setw(14) << "unique"
              << std::setw(9)  << "dup%"
              << std::setw(9)  << "undef" << '\n';

    std::vector<uint64_t> total_hashes;
    std::array<uint8_t, 32> record;

    size_t total_records = 0, total_undef = 0, total_unique = 0;

    for (auto& [dir, files] : dir_files) {
        std::vector<uint64_t> dir_hashes;
        size_t records = 0, undef = 0;

        for (const auto& fp : files) {
            std::ifstream input(fp, std::ios_base::binary);

            if (!input) {
                std::cout << "Failed to open " << fp << std::endl;
                continue;
            }

            while (input.read(reinterpret_cast<char*>(record.data()), record.size())
                         .gcount() == static_cast<std::streamsize>(record.size())) {
                int16_t score;
                std::memcpy(&score, record.data() + 24, sizeof(score));

                records++;
                undef += std::abs(static_cast<int>(score)) == sc::Undef.value();
                dir_hashes.push_back(hashTrainingRecord(record));
            }
        }

        sortUnique(dir_hashes);

        const size_t unique = dir_hashes.size();
        const double dup = records
                         ? 100.0 - 100.0 * static_cast<double>(unique) / static_cast<double>(records)
                         : 0.0;

        std::cout << std::left << std::setw(60) << abbreviatePath(dir, 60) << std::right
                  << std::setw(14) << records
                  << std::setw(14) << unique
                  << std::setw(8)  << std::fixed << std::setprecision(2) << dup << '%'
                  << std::defaultfloat
                  << std::setw(9)  << undef << std::endl;

        total_records += records;
        total_undef   += undef;
        total_unique  += unique;

        total_hashes.insert(total_hashes.end(), dir_hashes.begin(), dir_hashes.end());
        std::vector<uint64_t>().swap(dir_hashes);
    }

    sortUnique(total_hashes);

    const size_t unique_total = total_hashes.size();
    const double dup_total = total_records
                           ? 100.0 - 100.0 * static_cast<double>(unique_total) / static_cast<double>(total_records)
                           : 0.0;

    std::cout << std::string(60, '-') << '\n';
    std::cout << "TOTAL records " << total_records
              << " | unique across all directories " << unique_total
              << " | duplicates " << std::fixed << std::setprecision(2) << dup_total << "%"
              << std::defaultfloat
              << " | undef " << total_undef << std::endl;

    if (total_records and unique_total * 4 < total_records) {
        std::cout << "WARNING: only " << unique_total << " of " << total_records
                  << " records are distinct. A network trained on this will memorise it "
                     "instead of learning from it - fix the seeding/opening diversity and "
                     "regenerate before training." << std::endl;
    }

    std::cout << "Note: 'unique across all directories' is exact, but keeps one 8-byte "
                 "hash per distinct record, so a fully distinct dataset needs ~8 bytes "
                 "per record of memory." << std::endl;
}

void UtilsProtocol::parsePerft(std::istringstream& strm) {
    static constexpr int DepthTestLimit = 3 + DebugBuild * 2;

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
        std::atomic<clk::milliseconds> total_duration_ms = 0_ms;

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

                    clk::milliseconds duration_ms;
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

        // Standard UCI commands
             if (token == "uci")                   parseUCI();
        else if (token == "ucinewgame")            parseNewGame();
        else if (token == "position")              parsePosition(strm);
        else if (token == "go")                    parseGo(strm);
        else if (token == "isready")               parseIsReady();
        else if (token == "options")               parseShowOptions();
        else if (token == "setoption")             parseSetOptions(strm);
        
        // Utility commands
        else if (token == "print")                 _pos.print();
        else if (token == "export_net")            parseNeuralNet(strm);
        else if (token == "rewrite_header")        parseRewriteNet(strm);
        else if (token == "view_positions")        parseShowPositions(strm);

        // Testing commands
        else if (token == "test_pack")             packedPositionTests();
        else if (token == "test_ccr_one_hour")     ccrOneHourTest();
        else if (token == "test_null_move")        nullMoveTest();
        else if (token == "test_see")              seeTests();
        else if (token == "test_pack_on")          parsePackedFile(strm);
        else if (token == "test_extpack_on")       parseExtPackedFile(strm);
        else if (token == "test_perft")            parsePerft(strm);
        else if (token == "test_bullet_format")    parseTestBulletFormat(strm);
        else if (token == "verify_session")        parseVerifySession(strm);

        // Load openings for 'self_play' or 'spsa'
        else if (token == "load_openings")         GlobOpeningGenerator.load();

        // Setup and start tournament
        else if (token == "self_play")             parseSelfPlay(_collector, strm);
        // Transform data to bullet format (bullet-trainer compatible)
        else if (token == "to_bullet_format")      parseBulletFormat(strm);
        // Shuffle bullet format data
		else if (token == "shuffle_data")          parseDataShuffles(strm);
        // Report how much of the generated dataset is actually distinct
        else if (token == "data_check")            parseDataCheck(strm);

        // Setup and start SPSA tuning
        else if (token == "spsa")                  parseSPSA(strm);

        // Debug commands
#if defined(_UCI_DEBUG_UTILS)
        else if (token == "see")                   parseSEE(strm);
        else if (token == "nneval")                parseNNEval(strm);
#endif

    } while (command != "quit");
}

} // namespace utils
