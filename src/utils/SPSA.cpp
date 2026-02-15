#include "SPSA.h"
#include "Process.hpp"
#include "frontend/UCI.hpp"

#include <sys/wait.h>
#include <sstream>

namespace Utils {
    
_FORCEINLINE constexpr enumLogLabel operator|(enumLogLabel s0, enumLogLabel s1) {
    return static_cast<enumLogLabel>(static_cast<uint8_t>(s0) | static_cast<uint8_t>(s1));
}

_FORCEINLINE void labelLog(std::ostream& is, uint8_t label, const std::string& str) {

#if !defined(DEBUG)
    if (lv & LOG_DEBUG) 
        return;
#endif

    if (label != LOG_NO_LABEL) {
        std::string labels;

        auto add_label = [&](uint8_t bit, const char* name) {
            if (label & bit) {
                if (!labels.empty()) 
                    labels += "|";
                labels += name;
                label &= ~bit;
            }
        };

        add_label(LOG_DEBUG,    "DEBUG");
        add_label(LOG_INFO,     "INFO");
        add_label(LOG_ENGINE_0, "PLAYER_0");
        add_label(LOG_ENGINE_1, "PLAYER_1");
        is << "[" << labels << "] ";
    }

    is << str << std::endl;
}

_FORCEINLINE void log(std::ostream& is, const std::string& str) {
    labelLog(is, LOG_NO_LABEL, str);
}

_FORCEINLINE std::istream& readline(std::istream& os, std::string& line) {
    return std::getline(os, line);
}

void SPSA_Tuning::start() {
    auto engine0 = spawnProcess();
    auto engine1 = spawnProcess();

    auto& is0 = *engine0.in;
    auto& os0 = *engine0.out;
    auto& is1 = *engine1.in;
    auto& os1 = *engine1.out;

    const auto& tunable_options = UniversalChessInterface::getTunableOptions();

    std::vector<SPSA_Parameter> theta;
    theta.reserve(tunable_options.size());

    auto tranform_func = [](OptionTunableParam option) {
        SPSA_Parameter param;

        param.name = option.str;
        param.value = option.getCurrentValue();
        param.min = option.value.min_value;
        param.max = option.value.max_value;
        param.r = 0.025;
        param.c = (param.max - param.min) / 12.;

        return param;
    };

    std::transform(tunable_options.begin(),
                   tunable_options.end(),
                   std::back_inserter(theta),
                   tranform_func);

    std::vector<SPSA_PackedParameter> theta_plus;
    std::vector<SPSA_PackedParameter> theta_minus;

    theta_plus.reserve(tunable_options.size());
    theta_minus.reserve(tunable_options.size());

    static constexpr uint IterCount = 11;

	SearchLimits limits;

    // Game parameters
	limits.depth = MaxDepth - 1; // no depth limit
    limits.nodes = 0; // no node limit
    limits.wtime = limits.btime = 2_s;
    limits.winc = limits.binc = 20_ms;

    _openings.load("src/assets/sets/Nunn_Openings.epd");

    tune(theta, theta_plus, theta_minus, IterCount, limits,
         os0, is0, os1, is1);

    {
        std::string msg = "quit";
        log(is0, msg);
        log(is1, msg);
    }

    int status;
    waitpid(engine0.pid, &status, 0);
    waitpid(engine1.pid, &status, 0);
}

void SPSA_Tuning::tune(std::vector<SPSA_Parameter>& params,
                       std::vector<SPSA_PackedParameter>& theta_plus,
                       std::vector<SPSA_PackedParameter>& theta_minus,
                       uint n, SearchLimits limits,
                       std::istream& engine_os0, std::ostream& engine_is0,
                       std::istream& engine_os1, std::ostream& engine_is1)
{
    const uint A = n / 10;

    static constexpr double Alpha = 0.602;
    static constexpr double Gamma = 0.101;

    const size_t param_count = params.size();

    const double PowFactor = std::pow(A + 1, Alpha);

    for (int i = 0; i < param_count; i++) {
        params[i].a = params[i].r * params[i].c * params[i].c * PowFactor;
    }

    std::ofstream log_file("spsa_log_feb14.txt", std::ios_base::app);

    for (int k = 0; k < n; k++) {

        labelLog(std::cout, LOG_INFO, "ITERATION K = " + std::to_string(k));

        for (SPSA_Parameter& param : params) {
            param.ak = param.a / std::pow(A + k + 1, Alpha);
            param.ck = param.c / std::pow(k + 1, Gamma);
            param.delta = static_cast<double>(2 * random<int>(0, 1) - 1);
        }

        std::vector<SPSA_Parameter>& theta = params;

        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_plus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value += param.delta * param.ck;
                            packed.value = std::clamp(param.value, param.min, param.max);
                            return packed;
                       });
        std::transform(theta.begin(), theta.end(),
                       std::back_inserter(theta_minus),
                       [&](const SPSA_Parameter& param) -> SPSA_PackedParameter {
                            SPSA_PackedParameter packed;
                            packed.name = &param.name;
                            packed.value -= param.delta * param.ck;
                            packed.value = std::clamp(param.value, param.min, param.max);
                            return packed;
                       });

        applyOptions(theta_plus, engine_os0, engine_is0, LOG_DEBUG | LOG_ENGINE_0);
        applyOptions(theta_minus, engine_os1, engine_is1, LOG_DEBUG | LOG_ENGINE_1);

        const float res = match(limits, engine_os0, engine_is0, engine_os1, engine_is1);

        for (SPSA_Parameter& param : params) {
            const double gradient = res / (2. * param.ck * param.delta);
            param.value += param.ak * gradient;
            param.value = std::clamp(param.value, param.min, param.max);
        }

        theta_plus.clear();
        theta_minus.clear();

        if (k % 10 == 0)
            writeCheckpoint(log_file, theta, k);

        labelLog(std::cout, LOG_DEBUG, "Game result: " + std::to_string(res));
    }
}

void SPSA_Tuning::writeCheckpoint(std::ofstream& file, 
                                  std::vector<SPSA_Parameter>& theta,
                                  uint k)
{
    std::stringstream ss;
    ss << "[CHECKPOINT] ITERATION K = " << k << '\n';

    for (SPSA_Parameter& param : theta) {
        ss << param.name << " = " << param.value << '\n';
    }

    labelLog(file, LOG_INFO, ss.str());
}

void SPSA_Tuning::applyOptions(const std::vector<SPSA_PackedParameter>& tunable_options,
                               std::istream& engine_os, std::ostream& engine_is,
                               enumLogLabel ret_msg_label) 
{
    for (const SPSA_PackedParameter& param : tunable_options) {
        // "setoption name OPTION_STR value OPTION_VALUE"
        std::stringstream cmd;
        cmd << "setoption name " << *param.name << " value " << std::to_string(param.value);

        log(engine_is, cmd.str());
        labelLog(std::cout, ret_msg_label, cmd.str());
    }
}

INLINE float SPSA_Tuning::match(SearchLimits limits,
                                std::istream& engine_os0, std::ostream& engine_is0,
                                std::istream& engine_os1, std::ostream& engine_is1)
{
    // Is, os are relative to the engines.
    // We're writing to os, reading from is.
    struct EnginePlayer {
        std::istream* os;
        std::ostream* is;
    };

    EnginePlayer player[2];

    // (is0, os0) engine is white player
    const bool plus_player_white = random<int>(0, 1);

    // mixing sides to move 
    if (plus_player_white) {
        player[WHITE] = { &engine_os0, &engine_is0 };
        player[BLACK] = { &engine_os1, &engine_is1 };
    } 
    else {
        player[WHITE] = { &engine_os1, &engine_is1 };
        player[BLACK] = { &engine_os0, &engine_is0 };
    }

    log(*player[WHITE].is, "ucinewgame");
    log(*player[BLACK].is, "ucinewgame");

    for (enumColor side : { WHITE, BLACK }) {
        log(*player[side].is, "isready");

        std::string line;
        while ((readline(*player[side].os, line), line != "readyok"));
    }

    Timer timer;
    const bool time_constraint = limits.wtime != 0 and limits.btime != 0;

    const Position& opening = _openings.getPosition();
    const std::string start_fen = opening.createFEN();

    Game game(opening, time_constraint, limits.wtime, limits.btime);
    Game::Result game_result;

    static constexpr time_ms_t MoveOverhead = 15_ms;

    uint draw_full_moves = 0;

    enumLogLabel debug_labels[2];
    
    debug_labels[0] = plus_player_white ? LOG_DEBUG | LOG_ENGINE_0 
                                        : LOG_DEBUG | LOG_ENGINE_1;
    debug_labels[1] = plus_player_white ? LOG_DEBUG | LOG_ENGINE_1 
                                        : LOG_DEBUG | LOG_ENGINE_0;

    while (!game.isWin(game_result) and !game.isDraw(game_result)) {
        Position& pos = game.getPosition();
        const bool side2move = pos.getTurn();
        EnginePlayer& curr_player = player[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        Score eval = Score::Undef;
        sentPosition(start_fen, record, 
                     *curr_player.os, 
                     *curr_player.is,
                     debug_labels[side2move]);

        timer.go();
        Move32b move = getPlayerMove(limits, pos, 
                                     *curr_player.os, 
                                     *curr_player.is, 
                                     eval,
                                     debug_labels[side2move]);
        time_ms_t think_time = timer.duration();

        game.applyMove(move, think_time);

        if (time_constraint and side2move == WHITE) {
            limits.wtime -= think_time - limits.winc;
            limits.wtime += MoveOverhead;
        }
        else if (time_constraint) {
            limits.btime -= think_time - limits.binc;
            limits.btime += MoveOverhead;
        }

        ASSERTNOLOG(eval != Score::Undef);

        if (std::abs(static_cast<int>(eval)) < 90) 
            draw_full_moves += side2move;
        else
            draw_full_moves = 0;

        // Adjucate game as draw
        if (draw_full_moves > 35) {
            game_result = Game::DRAW_BY_ADJUCATION;
            break;
        }
    }

    //  1. - if player 0 wins
    // -1. - if player 1 wins

    if (isWhiteWin(game_result)) {
        return plus_player_white ? 1.f : -1.f;
    }
    else if (isBlackWin(game_result)) {
        return plus_player_white ? -1.f : 1.f;
    }
    return 0.f;
}

void SPSA_Tuning::sentPosition(const std::string& start_fen, 
                               const FullInfoRecord& record,
                               std::istream& engine_os, std::ostream& engine_is,
                               enumLogLabel ret_msg_label) 
{
    // Is, os are relative to the engines.
    // We're writing to os, reading from is.

    const int curr_halfmove_clock = record.currentHalfCount();

    std::stringstream cmd;
    cmd << "position fen " << start_fen;

    if (curr_halfmove_clock > 0)
        cmd << " moves";

    for (int halfmove_clock = 0;
         halfmove_clock < curr_halfmove_clock;
         halfmove_clock++) 
    {
        Move32b move = record.getPrevMove(halfmove_clock);
        cmd << " " << move;
    }

    const std::string msg = cmd.str();
    log(engine_is, msg);
    labelLog(std::cout, ret_msg_label, msg);
}

Move32b SPSA_Tuning::getPlayerMove(SearchLimits limits,
                                   const Position& pos,
                                   std::istream& engine_os, std::ostream& engine_is, 
                                   Score& score,
                                   enumLogLabel ret_msg_label) 
{
    // is, os streams are relative to the engines.
    // We're writing to is, reading from os.

    std::stringstream cmd;

    cmd << "go";
    cmd << " depth " << limits.depth;
    cmd << " wtime " << limits.wtime 
        << " btime " << limits.btime 
        << " winc "  << limits.winc 
        << " binc "  << limits.binc;
    
    log(engine_is, cmd.str());
    labelLog(std::cout, ret_msg_label, cmd.str());

    std::string line;
    std::string bestMoveStr;

    while (readline(engine_os, line)) {
        labelLog(std::cout, ret_msg_label, line);

        if (line.empty()) 
            continue;

        std::stringstream ss(line);
        std::string header;
        ss >> header;

        if (header == "info") {
            std::string word;

            while (ss >> word) {
                if (word == "score") {
                    std::string type;
                    int value;
                    ss >> type >> value;

                    if (type == "cp") {
                        score = static_cast<Score>(value);
                    } 
                    else if (type == "mate") {
                        score = (value >= 0) ? (Score::Mate - value) 
                                             : (-Score::Mate - value);
                    }
                }
            }
        } 
        else if (header == "bestmove") {
            ss >> bestMoveStr;
            break;
        }
    }

    return Move32b::fromStr<Move32b::Notation::REGULAR>(pos, bestMoveStr);
}

}
