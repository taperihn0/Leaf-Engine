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
#include "Process.hpp"
#include "frontend/UCI.hpp"
#include "SelfGame.hpp"
#include "Sets.hpp"

#include <sstream>
#include <atomic>
#include <mutex>

namespace Utils {

static constexpr uint   IterCount = 6000;
static constexpr int    A = IterCount / 10;
static constexpr double Alpha = 0.602;
static constexpr double Gamma = 0.101;

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
                   [](OptionTunableParam option) {
                        SPSA_Parameter param;

                        param.name = option.str;
                        param.value = option.getCurrentValue();
                        param.min = option.value.min_value;
                        param.max = option.value.max_value;
                        param.r = 0.03 * option.rate;
                        param.c = (param.max - param.min) / 12.;

                        return param;
                    });

    const double PowFactor = std::pow(A + 1, Alpha);

    for (int i = 0; i < param_count; i++) {
        params[i].a = params[i].r * params[i].c * params[i].c * PowFactor;
    }
    
    SearchLimits limits;

    // Game parameters
    limits.depth = MaxDepth; // avoid depth overflow
    limits.nodes = 0; // no node limit
    limits.wtime = limits.btime = 4_s;
    limits.winc = limits.binc = 100_ms;

    // TODO: try bigger opening set, maybe Lichess UHO?
    if (_openings.isEmpty())
        _openings.loadFromVec(NunnOpenings);

    std::ofstream log_file(spsa_log, std::ios_base::app);

    curr_iter.store(0);

    std::vector<std::thread> threads;

    for (uint id = 1; id <= thread_count; id++) {
        threads.emplace_back([&](std::vector<SPSA_Parameter>& theta, 
                                 std::ofstream& log_file, 
                                 SearchLimits limits,
                                 uint id) 
        {
            this->startThread(theta, log_file, limits, id);
        }, 
        std::ref(params), std::ref(log_file), limits, id);
    }

    for (auto& thread : threads)
        thread.join();
}

void SPSA_Tuning::startThread(std::vector<SPSA_Parameter>& theta, 
                              std::ofstream& log_file, 
                              SearchLimits limits,
                              uint id) 
{
    EngineProcess engine0;
    EngineProcess engine1;

    EngineProcess::initProc(engine0);
    EngineProcess::initProc(engine1);

    if (!engine0.isAlive() or !engine1.isAlive()) {
        labelLog(std::cout, LOG_INFO, "Process didn't initialize");
        return;
    }

    auto& is0 = *engine0.proc_stdin;
    auto& os0 = *engine0.proc_stdout;
    auto& is1 = *engine1.proc_stdin;
    auto& os1 = *engine1.proc_stdout;

    const size_t param_count = theta.size();

    std::vector<SPSA_PackedParameter> theta_plus;
    std::vector<SPSA_PackedParameter> theta_minus;

    theta_plus.reserve(param_count);
    theta_minus.reserve(param_count);

    const enumLogLabel thread_label = threadLabel(id);

    {
        engine0.syncUntilReady(thread_label);
        engine1.syncUntilReady(thread_label);

        std::string line;

#if defined(DEBUG)
        log(is0, "options");
        
        for (int i = 0; 
             i < param_count and readline(os0, line);
             i++) {
            labelLog(std::cout, LOG_DEBUG | LOG_ENGINE_0 | thread_label, line);
        }

        log(is1, "options");

        for (int i = 0; 
             i < param_count and readline(os1, line);
             i++) {
            labelLog(std::cout, LOG_DEBUG | LOG_ENGINE_1 | thread_label, line);
        }
#endif // DEBUG

        // set TT sizes
        
        static const size_t mb_tt_size = 32;

        std::stringstream tt_log;
        tt_log << "setoption name Hash value " << mb_tt_size;

        log(is0, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_0 | thread_label, tt_log.str());

        log(is1, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_1 | thread_label, tt_log.str());
    }

    tune(theta, theta_plus, theta_minus, 
         IterCount, 
         limits,
         engine0, engine1, 
         log_file, 
         id);

    {
        std::string msg = "quit";
        log(is0, msg);
        log(is1, msg);
    }

    engine0.waitForProcess();
    engine1.waitForProcess();
}

void SPSA_Tuning::tune(std::vector<SPSA_Parameter>& params,
                       std::vector<SPSA_PackedParameter>& theta_plus,
                       std::vector<SPSA_PackedParameter>& theta_minus,
                       uint n, SearchLimits limits,
                       EngineProcess& engine0,
                       EngineProcess& engine1,
                       std::ofstream& log_file,
                       uint id)
{
    const uint A = n / 10;
    const size_t param_count = params.size();

    uint theta_plus_win_cnt = 0;
    uint theta_minus_win_cnt = 0;
    uint draw_cnt = 0;

    ASSERT_NOLOG(id < static_cast<uint>(PlatformThreadLimit));
    const enumLogLabel curr_thread_label = threadLabel(id);

    while (true) {
        const uint k = curr_iter.fetch_add(1);

        if (k >= IterCount) break;

        labelLog(std::cout, LOG_INFO | curr_thread_label, "Iteration k = " + std::to_string(curr_iter));

        std::vector<SPSA_Parameter> local_params(param_count);

        {
            const std::lock_guard<std::mutex> lock(param_mutex);
            local_params = params;
        }

        for (SPSA_Parameter& param : local_params) {
            param.ak = param.a / std::pow(A + curr_iter + 1, Alpha);
            param.ck = param.c / std::pow(curr_iter + 1, Gamma);
            param.delta = static_cast<double>(2 * random<int>(0, 1) - 1);
        }

        std::vector<SPSA_Parameter>& theta = local_params;

        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_plus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value = param.value + param.delta * param.ck;
                            packed.value = std::clamp(packed.value, param.min, param.max);
                            return packed;
                       });
        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_minus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value = param.value - param.delta * param.ck;
                            packed.value = std::clamp(packed.value, param.min, param.max);
                            return packed;
                       });

        applyOptions(theta_plus, engine0, LOG_INFO | LOG_ENGINE_0 | curr_thread_label);
        applyOptions(theta_minus, engine1, LOG_INFO | LOG_ENGINE_1 | curr_thread_label);

        auto game_result = std::make_shared<Game::Result>();
        const int res = match(limits, 
                              engine0, engine1, 
                              game_result, id, curr_thread_label);

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

            if (k % 10 == 0)
                writeCheckpoint(log_file, params, k);
        }

        labelLog(std::cout, LOG_INFO | curr_thread_label, "Game info: " + toStr(*game_result) + 
                                                          ", numeric: " + std::to_string(res));
        
        std::stringstream info;
        info << "Theta Plus Wins | Theta Minus Wins | Draws: " 
             << theta_plus_win_cnt << " | "
             << theta_minus_win_cnt << " | "
             << draw_cnt;

        labelLog(std::cout, LOG_INFO | curr_thread_label, info.str());

        theta_plus.clear();
        theta_minus.clear();
    }
}

void SPSA_Tuning::writeCheckpoint(std::ofstream& file, 
                                  std::vector<SPSA_Parameter>& theta,
                                  uint k)
{
    std::stringstream ss;
    ss << "[CHECKPOINT] Iteration K = " << k << '\n';

    for (SPSA_Parameter& param : theta) {
        ss << param.name << " = " << param.value << '\n';
    }

    labelLog(file, LOG_INFO, ss.str());
}

void SPSA_Tuning::applyOptions(const std::vector<SPSA_PackedParameter>& tunable_options,
                               EngineProcess& engine,
                               enumLogLabel ret_msg_label) 
{
    // Setup option value using "setoption name OPTION value VALUE"

    for (const SPSA_PackedParameter& param : tunable_options) {
        std::stringstream cmd;
        cmd << "setoption name " << *param.name << " value " << std::to_string(param.value);

        log(*engine.proc_stdin, cmd.str());
        labelLog(std::cout, ret_msg_label, cmd.str());
    }
}

_INLINE int SPSA_Tuning::match(SearchLimits limits,
                               EngineProcess& engine0,
                               EngineProcess& engine1,
                               std::shared_ptr<Game::Result> result,
                               uint id,
                               enumLogLabel thread_label)
{
    SelfGame::GameSpecPacket game_packet = {
        limits,
        id,
        result,
        &_openings,
        nullptr,
    };

    engine0.syncUntilReady(thread_label);
    engine1.syncUntilReady(thread_label);

    const auto game_result = SelfGame().mixedMatch<_EnableSelfPlayLog>(engine0, engine1, game_packet);

    //  1. - if player zero wins
    // -1. - if player one wins
    //  0  - otherwise (draw or invalid game)

    if (isZeroPlayerWin(game_result))
        return 1;
    else if (isOnePlayerWin(game_result))
        return -1;

    return 0;
}

} // namespace Utils
