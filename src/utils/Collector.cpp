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

#include "Collector.hpp"
#include "PackedPosition.hpp"
#include "Process.hpp"
#include "SelfGame.hpp"
#include "TrainEntry.hpp"
#include "PackedNetwork.hpp"
#include "NetworkEval.hpp"
#include "StaticEval.hpp"

#include <thread>
#include <iomanip>

namespace Utils
{

bool TournamentCollector::threadTournamentWorker(TournamentCollector::PerThreadData& thr_data, 
                                                 enumLogLabel thread_label) 
{
    ASSERT(thr_data.output_white_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_black_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_draw.is_open(), "Output not opened.");
    ASSERT(thr_data.commons != nullptr, "Thread commons not initialized");

    EngineProcess engine0;
    EngineProcess engine1;

    EngineProcess::initProc(engine0);
    EngineProcess::initProc(engine1);

    if (!engine0.isAlive() or !engine1.isAlive()) {
        labelLog(std::cout, LOG_INFO | thread_label, "Process didn't initialize");
        return false;
    }

    {
        engine0.syncUntilReady(thread_label);
        engine1.syncUntilReady(thread_label);

        static const size_t mb_tt_size = 8;

        std::ostringstream tt_log;
        tt_log << "setoption name Hash value " << mb_tt_size;

        log(*engine0.proc_stdin, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_0 | thread_label, tt_log.str());

        log(*engine1.proc_stdin, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_1 | thread_label, tt_log.str());
    }

    engine0.syncUntilReady(thread_label);
    engine1.syncUntilReady(thread_label);

    std::vector<Position> positions;
    positions.reserve(MaxGameMoves);
    
    std::vector<Score> white_scores;
    white_scores.reserve(MaxGameMoves);
    
    std::vector<Move32b> moves;
    moves.reserve(MaxGameMoves);

    thr_data.games_ended = 0;
    thr_data.total_positions = 0;
    thr_data.white_win_count = 0;
    thr_data.black_win_count = 0;
    thr_data.draw_count = 0;

    const ll nodes_per_search = thr_data.limits.nodes;
    const ll nodes_randomization_range = static_cast<ll>(nodes_per_search * _NodesRandomFactor);
    const ll nodes_min = nodes_per_search - nodes_randomization_range;
    const ll nodes_max = nodes_per_search + nodes_randomization_range;

    auto train_data_spec = std::make_shared<SelfGame::TrainDataSpec>();
    train_data_spec->positions_buf = &positions;
    train_data_spec->white_scores_buf = &white_scores;
    train_data_spec->moves_buf = &moves;

    auto game_result = std::make_shared<Game::Result>(Game::GAME_INVALID);

    SelfGame::GameSpecPacket game_packet = {
        thr_data.limits,
        thr_data.id,
        game_result,
        &GlobOpeningGenerator,
        train_data_spec
    };

    const float stddev = _NodesRandomFactor / 2.f * nodes_per_search;
    std::normal_distribution normal_distr(static_cast<float>(nodes_per_search), stddev);
    std::mt19937 mt{GlobRandomSeed};

    for (size_t i = 0; 
         thr_data.commons->games_ended < thr_data.commons->games2play; 
         i++) 
    {
        {
            std::ostringstream ss;
            ss << "White wins, Black wins, Draws: "
               << thr_data.white_win_count << ' '
               << thr_data.black_win_count << ' '
               << thr_data.draw_count;
            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        {
            std::ostringstream ss;
            ss << "Starting game " << i << "...";
            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        // add noise to search node number (if nodes threshold is used)
        if (nodes_per_search > 0) {
            game_packet.limits.nodes = std::clamp(std::llroundf(normal_distr(mt)), nodes_min, nodes_max);
        }

        *game_result = Game::GAME_INVALID;

        if (!engine0.isAlive() or !engine1.isAlive()) {
            {
                const std::lock_guard<std::mutex> lock(thr_data.commons->err_output_lock);
                labelLog(thr_data.commons->err_output, LOG_INFO | thread_label, "Engine disconnected");
            }

            engine0.waitForProcess();
            engine1.waitForProcess();

            return false;
        }

        engine0.syncUntilReady(thread_label);
        engine1.syncUntilReady(thread_label);

        positions.clear();
        white_scores.clear();
        moves.clear();

        SelfGame().mixedMatch<_EnableSelfPlayLog>(engine0, engine1, game_packet);

        const size_t total_positions_cnt = positions.size();
        ASSERT_NOLOG(total_positions_cnt == white_scores.size() and 
                    total_positions_cnt == moves.size());

        if (*game_result == Game::GAME_INVALID) {
            const std::lock_guard<std::mutex> lock(thr_data.commons->err_output_lock);
            labelLog(thr_data.commons->err_output, LOG_INFO | thread_label, "Error: Invalid game");
            
            std::ostringstream ss;
            ss  << " nodes " << game_packet.limits.nodes
                << " qnodes "<< game_packet.limits.qnodes
                << " depth " << game_packet.limits.depth
                << " wtime " << game_packet.limits.wtime
                << " btime " << game_packet.limits.btime 
                << " winc "  << game_packet.limits.winc 
                << " binc "  << game_packet.limits.binc;

            labelLog(thr_data.commons->err_output, LOG_INFO | thread_label, "Game specs: " + ss.str());

            for (size_t i = 0; i < total_positions_cnt; i++) {
                positions[i].print(thr_data.commons->err_output);
                thr_data.commons->err_output << "Following move: ";
                moves[i].print(thr_data.commons->err_output);
                thr_data.commons->err_output << "\nWhite-POV Search Score: " 
                                             << static_cast<int16_t>(white_scores.at(i))
                                             << '\n';
            }

            thr_data.commons->err_output << std::endl;
            labelLog(std::cout, LOG_INFO | thread_label, "Invalid game occured");
            return false;
        }

        uint filtered_positions_cnt = 0;

        const TrainingDataEntry::Result8b result8b = isWhiteWin(*game_result) ? TrainingDataEntry::WHITE_WIN :
                                                     isBlackWin(*game_result) ? TrainingDataEntry::BLACK_WIN :
                                                                                TrainingDataEntry::DRAW;

        for (size_t i = 0; i < positions.size(); i++) {
            Position& pos = positions[i];
            const Score white_score = white_scores[i];

            assert(moves[i].isLegal(pos));

            if (!filterTrainPosition(pos, white_score, moves[i], total_positions_cnt))
                continue;

            filtered_positions_cnt++;

            const TrainingDataEntry entry(PackedPosition::packed(pos), white_score, result8b);

            if (isWhiteWin(*game_result) and 
                !TrainingDataEntry::write(thr_data.output_white_win, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
            
            else if (isBlackWin(*game_result) and
                     !TrainingDataEntry::write(thr_data.output_black_win, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
            
            else if (isDraw(*game_result) and 
                     !TrainingDataEntry::write(thr_data.output_draw, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
        }

        thr_data.total_positions += filtered_positions_cnt;
        thr_data.commons->total_positions.fetch_add(filtered_positions_cnt);

        {
            std::ostringstream ss;
            ss << toStr(*game_result) << " - collected " << filtered_positions_cnt << " positions";

            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        if (isWhiteWin(*game_result)) {
            thr_data.white_win_count++;
            thr_data.commons->total_white_win_count.fetch_add(1);
        }

        else if (isBlackWin(*game_result)) {
            thr_data.black_win_count++;
            thr_data.commons->total_black_win_count.fetch_add(1);
        }

        else if (isDraw(*game_result)) {
            thr_data.draw_count++;
            thr_data.commons->total_draw_count.fetch_add(1);
        }

        else if (*game_result == Game::GAME_INVALID)
            break;

        thr_data.games_ended++;
        thr_data.commons->games_ended.fetch_add(1);

        labelLog(std::cout, LOG_INFO | thread_label, 
                    "Total of " + std::to_string(thr_data.games_ended) + " games played on thread");
        labelLog(std::cout, LOG_INFO | thread_label, 
                    "Total of " + std::to_string(thr_data.total_positions) + " positions collected on thread");
        labelLog(std::cout, LOG_INFO, 
                    "Total of " + std::to_string(thr_data.commons->games_ended) + " games played on all threads");
        labelLog(std::cout, LOG_INFO, 
                    "Total of " + std::to_string(thr_data.commons->total_positions) + " positions collected on all threads");

#if defined(INSPECT_SELFPLAY_MATCHES)
        if (thr_data.commons->total_thread_cnt == 1) {            
            positions.back().print();
            labelLog(std::cout, LOG_INFO | thread_label, toStr(*game_result) + ": ");

            for (size_t i = 0; i < std::min<size_t>(120, white_scores.size()); i++)
                std::cout << static_cast<int16_t>(white_scores.at(i)) << ' ';

            std::cout << std::endl;
            labelLog(std::cout, LOG_INFO | thread_label, "Enter to continue tournament...");
            std::cin.get();
        }
        else {
            labelLog(std::cout, LOG_DEBUG | thread_label, 
                     "Self-play game inspection avaible only for 1 thread tournament");
        }
#endif
    }

    {
        std::ostringstream ss;
        ss << thr_data.games_ended << " games played - total of " 
           << thr_data.total_positions 
           << " positions collected.";
        labelLog(std::cout, LOG_INFO | thread_label, ss.str());
    }

    if (engine0.isAlive())
        log(*engine0.proc_stdin, "quit");
    
    if (engine1.isAlive())
        log(*engine1.proc_stdin, "quit");

    engine0.waitForProcess();
    engine1.waitForProcess();

    return true;
}

void TournamentCollector::perThread(TournamentCollector::PerThreadData& thr_data) {
    const enumLogLabel thread_label = threadLabel(thr_data.id);

    while (!threadTournamentWorker(thr_data, thread_label)) {
        labelLog(std::cout, LOG_INFO | thread_label, "Restarting tournament and engines on thread");
    }

    labelLog(std::cout, LOG_INFO | thread_label, "Terminating thread");
}

void TournamentCollector::startTournament(const TournamentPacket& packet) {
    if (packet.thread_count > static_cast<uint>(PlatformThreadLimit)) {
        std::cout << "Too many threads requested" << std::endl;
        return;
    }

    auto thread_common = std::make_shared<CommonThreadData>();

    thread_common->games_ended = 0;
    thread_common->games2play = packet.games_count;
    thread_common->total_positions = 0;
    thread_common->total_white_win_count = 0;
    thread_common->total_black_win_count = 0;
    thread_common->total_draw_count = 0;
    thread_common->total_thread_cnt = packet.thread_count;

    thread_common->err_output.open(packet.err_log_dir, std::ios::app);

    if (!thread_common->err_output) {
        labelLog(std::cout, LOG_INFO, "Failed to error file open");
        return;
    }

    if (GlobOpeningGenerator.isEmpty())
        GlobOpeningGenerator.load();

    std::vector<PerThreadData> thread_private;
    std::vector<std::thread> threads;

    thread_private.reserve(packet.thread_count);
    threads.reserve(packet.thread_count);

    for (uint id = 1; id <= packet.thread_count; id++) {
        PerThreadData per_thread_data;

        {
            const std::filesystem::path fp = packet.log_dir / getWhiteWinOutputFile(id);
            per_thread_data.output_white_win.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_white_win) {
                labelLog(std::cout, LOG_INFO, "Failed to open " + fp.string());
                return;
            }
        }
        {
            const std::filesystem::path fp = packet.log_dir / getBlackWinOutputFile(id);
            per_thread_data.output_black_win.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_black_win) {
                labelLog(std::cout, LOG_INFO, "Failed to open " + fp.string());
                return;
            }
        }
        {
            const std::filesystem::path fp = packet.log_dir / getDrawOutputFile(id);
            per_thread_data.output_draw.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_draw) {
                labelLog(std::cout, LOG_INFO, "Failed to open " + fp.string());
                return;
            }
        }

        per_thread_data.limits = packet.limits;
        per_thread_data.commons = thread_common;
        per_thread_data.id = id;

        thread_private.push_back(std::move(per_thread_data));
    }

    std::ostringstream ss;
    ss << "Starting " << packet.thread_count << " threads";

    labelLog(std::cout, LOG_INFO, ss.str());

    for (size_t i = 0; i < packet.thread_count; i++) {
        threads.emplace_back([this](PerThreadData& thread_data) {
            this->perThread(thread_data);
        }, 
        std::ref(thread_private[i]));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    labelLog(std::cout, LOG_INFO, "Tournament finished.");
    
    ss.str("");
    ss << "White wins, Black wins, Draws: "
        << thread_common->total_white_win_count << ' '
        << thread_common->total_black_win_count << ' '
        << thread_common->total_draw_count;

    labelLog(std::cout, LOG_INFO, ss.str());

    labelLog(std::cout, LOG_INFO, 
             "Total of " + std::to_string(thread_common->games_ended) + " games played on all threads");
    labelLog(std::cout, LOG_INFO, 
             "Total of " + std::to_string(thread_common->total_positions) + " positions collected on all threads");
}

bool TournamentCollector::explicitFilterPolicy(const Position& pos, 
                                               Score white_score) 
{
    if (white_score.isMateScore())
        return false;

    else if (pos.getPiecesCount() <= 6 and 
             StaticEval::evaluateEndgame(pos) != Score::Undef)
        return false;

    else if (pos.isInCheck(pos.getTurn()))
        return false;

    return true;
}

_FORCEINLINE bool TournamentCollector::internalFilterPolicy(Move32b internal_move,
                                                            _UNUSED size_t internal_total_positions_cnt)
{
    if (internal_move.isCapture() or internal_move.isPromotion())
        return false;

    return true;
}

_FORCEINLINE bool TournamentCollector::filterTrainPosition(const Position& pos, 
                                                            Score white_score,
                                                            Move32b internal_move,
                                                            size_t internal_total_positions_cnt) 
{
    return explicitFilterPolicy(pos, white_score) and 
           internalFilterPolicy(internal_move, internal_total_positions_cnt);
}

} // namespace Utils
