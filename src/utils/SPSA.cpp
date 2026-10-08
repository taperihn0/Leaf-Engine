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

#include "SPSA.hpp"
#include "Log.hpp"
#include "Process.hpp"
#include "frontend/UCI.hpp"
#include "SelfGame.hpp"
#include "Sets.hpp"

#include <sstream>
#include <atomic>
#include <mutex>

namespace utils {

static constexpr uint   IterCount = 6000;
static constexpr int    A = IterCount / 10;
static constexpr double Alpha = 0.602;
static constexpr double Gamma = 0.101;

static constexpr float FloatEpsilon = 10e-5f;

std::atomic<int> curr_iter;
std::mutex       param_mutex;

void SPSA_Tuning::start(uint thread_count, const std::filesystem::path& spsa_log) {
    if (thread_count > static_cast<uint>(PlatformThreadLimit)) {
        std::cout << "Too many threads requested" << std::endl;
        return;
    }

    const auto& tunable_options = UniversalChessInterface::getTunableOptions();
    const size_t param_count = tunable_options.size();

    std::vector<SPSA_Parameter> params;
    params.reserve(param_count);

    std::transform(tunable_options.begin(),
                   tunable_options.end(),
                   std::back_inserter(params),
                   [](opt::OptionTunableParam option) {
                        SPSA_Parameter param;

                        param.name = option.str;
                        param.value = option.getCurrentValue();
                        param.min = option.value.min_value;
                        param.max = option.value.max_value;
                        param.r = 0.04 * option.rate;
                        param.c = (param.max - param.min) / 12.;

                        return param;
                    });

    const double PowFactor = std::pow(A + 1, Alpha);

    for (int i = 0; i < param_count; i++) {
        params[i].a = params[i].r * params[i].c * params[i].c * PowFactor;
    }
    
    search::utils::SearchLimits limits;

    // Game parameters
    limits.depth = MaxDepth; // avoid depth overflow
    limits.nodes = 0; // no node limit
    limits.wtime = limits.btime = 8_s;
    limits.winc = limits.binc = 80_ms;

    if (_openings.isEmpty())
        _openings.loadFromVec(getLichessUHO_Openings());

    std::ofstream log_file(spsa_log, std::ios_base::app);

    curr_iter.store(0);

    std::vector<std::thread> threads;

    for (uint id = 1; id <= thread_count; id++) {
        threads.emplace_back([&](std::vector<SPSA_Parameter>& theta, 
                                 std::ofstream& log_file, 
                                 search::utils::SearchLimits limits,
                                 uint id) 
        {
            this->startThread(theta, log_file, limits, id);
        }, 
        std::ref(params), std::ref(log_file), limits, id);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

void SPSA_Tuning::startThread(std::vector<SPSA_Parameter>& theta, 
                              std::ofstream& log_file, 
                              search::utils::SearchLimits limits,
                              uint id) 
{
    PairOfForks competitors;

    for (auto& engine : competitors) {
        if (!engine.isAlive()) {
            lg::warning("Process didn't initialize");
            return;
        }
    }

    const size_t param_count = theta.size();

    std::vector<SPSA_PackedParameter> theta_plus;
    std::vector<SPSA_PackedParameter> theta_minus;

    theta_plus.reserve(param_count);
    theta_minus.reserve(param_count);

    const lg::logLabel log_thr_label = lg::threadLabel(id);

    {
        for (auto& engine : competitors) {
            engine.syncUntilReady(log_thr_label);
        }

        std::string line;

#if defined(DEBUG)
        for (auto& engine : competitors) {
            engine.output().message("options");

            for (int i = 0; 
                i < param_count and readline(engine.input(), line);
                i++) {
                lg::debug(log_thr_label, line);
            }
        }
#endif // DEBUG

        // set TT sizes
        
        static const size_t mb_tt_size = 32;

        for (auto& engine : competitors) {
            engine.output().message("setoption name Hash value ", mb_tt_size);
            lg::info(log_thr_label, "setoption name Hash value ", mb_tt_size);
        }
    }

    tune(theta, theta_plus, theta_minus, 
         IterCount, 
         limits,
         competitors,
         log_file, 
         id);

    for (auto& engine : competitors) {
        engine.output().message("quit");
        engine.waitForProcess();
    }
}

void SPSA_Tuning::tune(std::vector<SPSA_Parameter>& params,
                       std::vector<SPSA_PackedParameter>& theta_plus,
                       std::vector<SPSA_PackedParameter>& theta_minus,
                       uint n, search::utils::SearchLimits limits,
                       PairOfForks& competitors,
                       std::ofstream& log_file,
                       uint id)
{
    const uint A = n / 10;
    const size_t param_count = params.size();

    uint theta_plus_win_cnt = 0;
    uint theta_minus_win_cnt = 0;
    uint draw_cnt = 0;

    ASSERT_NO_LOG(id < static_cast<uint>(PlatformThreadLimit));
    const lg::logLabel curr_log_thr_label = lg::threadLabel(id);

    lg::Log log(log_file);

    while (true) {
        const uint k = curr_iter.fetch_add(1);

        if (k >= IterCount) break;

        lg::info(curr_log_thr_label, "Iteration k = ", curr_iter.load());

        std::vector<SPSA_Parameter> local_params(param_count);

        {
            const std::lock_guard<std::mutex> lock(param_mutex);
            local_params = params;
        }

        for (SPSA_Parameter& param : local_params) {
            param.ak = param.a / std::pow(A + curr_iter + 1, Alpha);
            param.ck = param.c / std::pow(curr_iter + 1, Gamma);
            param.delta = static_cast<double>(2 * rnd::random<int>(0, 1) - 1);
        }

        std::vector<SPSA_Parameter>& theta = local_params;

        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_plus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value = param.value + param.delta * param.ck;
                            packed.value = std::clamp(packed.value, param.min + FloatEpsilon, param.max - FloatEpsilon);
                            return packed;
                       });

        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_minus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value = param.value - param.delta * param.ck;
                            packed.value = std::clamp(packed.value, param.min + FloatEpsilon, param.max - FloatEpsilon);
                            return packed;
                       });

        applyOptions(theta_plus, competitors.getFork(0), curr_log_thr_label);
        applyOptions(theta_minus, competitors.getFork(1), curr_log_thr_label);

        SelfGame::GameSpecPacket game_packet = {
            limits,
            id,
            Game::GAME_INVALID,
            &_openings,
            nullptr,
        };

        std::allocator<Game> al;
        using altraits = std::allocator_traits<decltype(al)>;
        
        Game* game = altraits::allocate(al, 1);

        const int res = matchWrapper(competitors,
                                     game_packet,
                                     game,
                                     curr_log_thr_label);

        if (res == 1) {
            theta_plus_win_cnt++;
        } 
        else if (res == -1) {
            theta_minus_win_cnt++;
        } 
        else draw_cnt++;

        {
            const std::lock_guard<std::mutex> lock(param_mutex);

            for (int i = 0; i < param_count; i++) {
                const double gradient = static_cast<double>(res) / (2. * local_params[i].ck * local_params[i].delta);
                params[i].value += local_params[i].ak * gradient;
                params[i].value = std::clamp(params[i].value, params[i].min, params[i].max);
            }

            if (k % 10 == 0) {
                writeCheckpoint(log, params, k);
            }
        }

        lg::info(curr_log_thr_label, "Game info: ", game_packet.result, ", numeric: ", res);
        
        std::ostringstream info;
        info << "Theta Plus Wins | Theta Minus Wins | Draws: " 
             << theta_plus_win_cnt << " | "
             << theta_minus_win_cnt << " | "
             << draw_cnt;

        lg::info(curr_log_thr_label, info.str());

        theta_plus.clear();
        theta_minus.clear();
    }
}

void SPSA_Tuning::writeCheckpoint(lg::Log& log, 
                                  std::vector<SPSA_Parameter>& theta,
                                  uint k)
{
    std::ostringstream ss;
    ss << "[CHECKPOINT] Iteration K = " << k << '\n';

    for (SPSA_Parameter& param : theta) {
        ss << param.name << " = " << param.value << '\n';
    }

    log.message(ss.str());
}

void SPSA_Tuning::applyOptions(const std::vector<SPSA_PackedParameter>& tunable_options,
                               ForkedProcess& engine,
                               lg::logLabel log_thr_label) 
{
    // Setup option value using "setoption name OPTION value VALUE"

    for (const SPSA_PackedParameter& param : tunable_options) {
        std::ostringstream cmd;
        cmd << "setoption name " << *param.name << " value " << param.value;

        engine.output().message(cmd.str());
        lg::info(log_thr_label, cmd.str());
    }
}

_INLINE int SPSA_Tuning::matchWrapper(PairOfForks& competitors,
                                      SelfGame::GameSpecPacket& game_packet,
                                      Game* game,
                                      lg::logLabel log_thr_label)
{
    for (auto& engine : competitors) {
        engine.syncUntilReady(log_thr_label);
    }

    const auto game_result = SelfGame::get()
        .mixedMatch<_EnableSelfPlayLog>(competitors, game_packet, game);

    //  1. - if player zero wins
    // -1. - if player one wins
    //  0  - otherwise (draw or invalid game)

    if (SelfGame::isZeroPlayerWin(game_result))
        return 1;
    else if (SelfGame::isOnePlayerWin(game_result))
        return -1;

    return 0;
}

} // namespace utils
