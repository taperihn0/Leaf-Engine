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
#include "Log.hpp"
#include "PackedPosition.hpp"
#include "Process.hpp"
#include "SelfGame.hpp"
#include "TrainEntry.hpp"
#include "PackedNetwork.hpp"
#include "NetworkEval.hpp"
#include "StaticEval.hpp"
#include "Paths.hpp"

#include <thread>
#include <iomanip>

namespace utils {

bool TournamentCollector::threadTournamentWorker(TournamentCollector::PerThreadData& thr_data, 
                                                 lg::logLabel log_thr_label) 
{
    ASSERT(thr_data.output_white_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_black_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_draw.is_open(), "Output not opened.");
    ASSERT(thr_data.commons != nullptr, "Thread commons not initialized");

    PairOfForks competitors;

    for (const auto& engine : competitors) {
        if (!engine.isAlive()) {
            lg::info(log_thr_label, "Process didn't initialize");
            return false;
        }
    }

    {
        static const size_t mb_tt_size = 8;

        for (auto& engine : competitors) {
            engine.syncUntilReady(log_thr_label);

            engine.output().message("setoption name Hash value ", mb_tt_size);
            lg::info(log_thr_label, "setoption name Hash value ", mb_tt_size);
            
            // TODO: SyzygyPath manual setup
        }
    }

    for (auto& engine : competitors) {
        engine.syncUntilReady(log_thr_label);
    }

    thr_data.games_ended = 0;
    thr_data.total_positions = 0;
    thr_data.white_win_count = 0;
    thr_data.black_win_count = 0;
    thr_data.draw_count = 0;

    const ll nodes_per_search = thr_data.limits.nodes;
    const ll nodes_randomization_range = static_cast<ll>(nodes_per_search * _NodesRandomFactor);
    const ll nodes_min = nodes_per_search - nodes_randomization_range;
    const ll nodes_max = nodes_per_search + nodes_randomization_range;

    SelfGame::GameSpecPacket game_packet = {
        thr_data.limits,
        thr_data.id,
        Game::GAME_INVALID,
        &GlobOpeningGenerator,
        std::make_shared<std::vector<SelfGame::PositionInfo>>()
    };

    game_packet.data_buffer->reserve(MaxGameMoves);

    const float stddev = _NodesRandomFactor / 2.f * nodes_per_search;
    std::normal_distribution normal_distr(static_cast<float>(nodes_per_search), stddev);
    std::mt19937_64 mt = rnd::getRandomEngine();

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
            lg::info(log_thr_label, ss.str());
        }

        {
            std::ostringstream ss;
            ss << "Starting game " << i << "...";
            lg::info(log_thr_label, ss.str());
        }

        // add noise to search node number (if nodes threshold is used)
        if (nodes_per_search > 0) {
            game_packet.limits.nodes = std::clamp(std::llroundf(normal_distr(mt)), nodes_min, nodes_max);
        }

        if (!competitors.raw().first.isAlive() or
            !competitors.raw().second.isAlive()) 
        {
            {
                const std::lock_guard<std::mutex> lock(thr_data.commons->err_output_lock);
                lg::Log(thr_data.commons->err_fp, std::ios::app)
                    .info(log_thr_label, "Engine disconnected");
            }

            competitors.raw().first.waitForProcess();
            competitors.raw().second.waitForProcess();

            return false;
        }

        for (auto& engine : competitors) {
            engine.syncUntilReady(log_thr_label);
        }

        game_packet.result = Game::GAME_INVALID;
        game_packet.data_buffer->clear();

        std::allocator<Game> al;
        using altraits = std::allocator_traits<decltype(al)>;
        
        Game* game = altraits::allocate(al, 1);
        _UNUSED const auto result = SelfGame::get().mixedMatch<_EnableSelfPlayLog>(competitors, game_packet, game);

        const size_t total_positions_cnt = game_packet.data_buffer->size();

        if (game_packet.result == Game::GAME_INVALID) {
            const std::lock_guard<std::mutex> lock(thr_data.commons->err_output_lock);

            lg::Log errlog(thr_data.commons->err_fp, std::ios::app);
            errlog.message(log_thr_label, "Error: Invalid game");
            
            std::ostringstream ss;
            ss  << " nodes " << game_packet.limits.nodes
                << " qnodes "<< game_packet.limits.qnodes
                << " depth " << game_packet.limits.depth
                << " wtime " << game_packet.limits.wtime
                << " btime " << game_packet.limits.btime 
                << " winc "  << game_packet.limits.winc 
                << " binc "  << game_packet.limits.binc;

            errlog.message(log_thr_label, "Game specs: ", ss.str());

            for (const auto& [pos, white_score, move] : *game_packet.data_buffer) {
                errlog.message(pos, "Following move: ", move);
                errlog.message("White-POV Search Score: ", white_score.value());
            }

            errlog.flush();
            lg::warning(log_thr_label, "Invalid game occured");
            return false;
        }

        uint filtered_positions_cnt = 0;

        const TrainingDataEntry::Result8b result8b = 
            isWhiteWin(game_packet.result) ? TrainingDataEntry::WHITE_WIN :
            isBlackWin(game_packet.result) ? TrainingDataEntry::BLACK_WIN :
                                             TrainingDataEntry::DRAW;

        for (auto& [pos, white_score, move] : *game_packet.data_buffer) {
            assert(move.isLegal(*pos));

            if (!filterTrainPosition(*pos, white_score, move, total_positions_cnt))
                continue;

            filtered_positions_cnt++;

            const TrainingDataEntry entry(PackedPosition::packed(*pos), white_score, result8b);

            if (isWhiteWin(game_packet.result) and 
                !TrainingDataEntry::write(thr_data.output_white_win, entry)) {
                lg::info(log_thr_label, "Failed to write entry");
            }
            
            else if (isBlackWin(game_packet.result) and
                     !TrainingDataEntry::write(thr_data.output_black_win, entry)) {
                lg::info(log_thr_label, "Failed to write entry");
            }
            
            else if (isDraw(game_packet.result) and 
                     !TrainingDataEntry::write(thr_data.output_draw, entry)) {
                lg::info(log_thr_label, "Failed to write entry");
            }
        }

        thr_data.total_positions += filtered_positions_cnt;
        thr_data.commons->total_positions.fetch_add(filtered_positions_cnt);

        {
            std::ostringstream ss;
            ss << toStr(game_packet.result) << " - collected " << filtered_positions_cnt << " positions";
            lg::info(log_thr_label, ss.str());
        }

        if (isWhiteWin(game_packet.result)) {
            thr_data.white_win_count++;
            thr_data.commons->total_white_win_count.fetch_add(1);
        }

        else if (isBlackWin(game_packet.result)) {
            thr_data.black_win_count++;
            thr_data.commons->total_black_win_count.fetch_add(1);
        }

        else if (isDraw(game_packet.result)) {
            thr_data.draw_count++;
            thr_data.commons->total_draw_count.fetch_add(1);
        }

        else if (game_packet.result == Game::GAME_INVALID)
            break;

        thr_data.games_ended++;
        thr_data.commons->games_ended.fetch_add(1);

        lg::info(log_thr_label, "Total of ", thr_data.games_ended, " games played on thread");
        lg::info(log_thr_label, "Total of ", thr_data.total_positions, " positions collected on thread");
        lg::info("Total of ", thr_data.commons->games_ended, " games played on all threads");
        lg::info("Total of ", thr_data.commons->total_positions, " positions collected on all threads");

#if defined(INSPECT_SELFPLAY_MATCHES)
        if (thr_data.commons->total_thread_cnt == 1) {            
            lg::message(log_thr_label, 
                        *game_packet.data_buffer->back().pos,
                        toStr(game_packet.result),
                        ": ");

            std::ostringstream ss;

            for (size_t i = 0; i < std::min<size_t>(120, game_packet.data_buffer->size()); i++) {
                const auto white_score = game_packet.data_buffer->at(i).white_score;
                ss << static_cast<int16_t>(white_score) << ' ';
            }

            lg::message(log_thr_label, ss.str());
            lg::message(log_thr_label, "Enter to continue tournament...");
            std::cin.get();
        }
        else {
            lg::message(lg::logLabel::LOG_DEBUG | log_thr_label, 
                        "Self-play game inspection avaible only for 1 thread tournament");
        }
#endif
    }

    {
        std::ostringstream ss;
        ss << thr_data.games_ended << " games played - total of " 
           << thr_data.total_positions 
           << " positions collected.";
        lg::info(log_thr_label, ss.str());
    }

    for (auto& engine : competitors) {
        if (engine.isAlive()) {
            engine.output().message("quit");
        }

        engine.waitForProcess();
    }

    return true;
}

void TournamentCollector::perThread(TournamentCollector::PerThreadData& thr_data) {
    const lg::logLabel log_thr_label = lg::threadLabel(thr_data.id);

    while (!threadTournamentWorker(thr_data, log_thr_label)) {
        lg::info(log_thr_label, "Restarting tournament and engines on thread");
    }

    lg::info(log_thr_label, "Terminating thread");
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
    thread_common->err_fp = packet.selfplay_filename / "err";

    if (GlobOpeningGenerator.isEmpty())
        GlobOpeningGenerator.load();

    std::vector<PerThreadData> thread_private;
    std::vector<std::thread> threads;

    thread_private.reserve(packet.thread_count);
    threads.reserve(packet.thread_count);

    for (uint id = 1; id <= packet.thread_count; id++) {
        PerThreadData per_thread_data;
        std::error_code err;

        if (std::filesystem::create_directories(packet.selfplay_filename, err), err) {
            lg::info("Failed to create directory: ", packet.selfplay_filename.string(),
                      ", reason: ", err.message());
            return;
        }

        {
            const std::filesystem::path fp = packet.selfplay_filename 
                                                / paths::PathsManager.getWhiteWinOutputFileName(id);

            per_thread_data.output_white_win.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_white_win) {
                lg::info("Failed to open ", fp.string());
                return;
            }
        }

        {
            const std::filesystem::path fp = packet.selfplay_filename 
                                                / paths::PathsManager.getBlackWinOutputFileName(id);

            per_thread_data.output_black_win.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_black_win) {
                lg::info("Failed to open ", fp.string());
                return;
            }
        }

        {
            const std::filesystem::path fp = packet.selfplay_filename 
                                                / paths::PathsManager.getDrawOutputFileName(id);

            per_thread_data.output_draw.open(fp, std::ios::binary | std::ios::app);

            if (!per_thread_data.output_draw) {
                lg::info("Failed to open ", fp.string());
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

    lg::info(ss.str());

    for (size_t i = 0; i < packet.thread_count; i++) {
        threads.emplace_back([this](PerThreadData& thread_data) {
            this->perThread(thread_data);
        }, 
        std::ref(thread_private[i]));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    lg::info("Tournament finished.");
    
    ss.str("");
    ss << "White wins, Black wins, Draws: "
        << thread_common->total_white_win_count << ' '
        << thread_common->total_black_win_count << ' '
        << thread_common->total_draw_count;

    lg::info(ss.str());
    lg::info("Total of ", thread_common->games_ended, " games played on all threads");
    lg::info("Total of ", thread_common->total_positions, " positions collected on all threads");
}

bool TournamentCollector::explicitFilterPolicy(const Position& pos, 
                                               sc::Score white_score) 
{
    if (white_score.isMateScore())
        return false;

    else if (pos.getPiecesCount() <= 6 and 
             hce::StaticEval::evaluatePawnlessEndgame(pos) != sc::Undef)
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
                                                           sc::Score white_score,
                                                           Move32b internal_move,
                                                           size_t internal_total_positions_cnt) 
{
    return explicitFilterPolicy(pos, white_score) and 
           internalFilterPolicy(internal_move, internal_total_positions_cnt);
}

} // namespace utils
