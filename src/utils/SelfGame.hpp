#pragma once

#include "UtilsCommon.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace Utils {

class SelfGame {
public:
    SelfGame() = default;

    /* Is, os are relative to the engines.
    *  We're writing to os, reading from is.
    */
    struct EnginePlayer {
        std::istream* os;
        std::ostream* is;
    };

    struct GameSpecPacket {
        SearchLimits                    limits;
        EnginePlayer                    engine0;
        EnginePlayer                    engine1;
        uint                            thread_id;
        std::shared_ptr<Game::Result>   result;
        OpeningManBase*                 openings; 
        std::shared_ptr<std::vector<PackedPosition>> positions_buf;
        std::shared_ptr<std::vector<Score>> white_scores_buf;
    };

    enum PlayerPerspectiveResult {
        GAME_INVALID,
        PLAYER_ZERO_WIN_BY_MATE,
        PLAYER_ONE_WIN_BY_MATE,
        PLAYER_ZERO_WIN_BY_ADJUCATION,
        PLAYER_ONE_WIN_BY_ADJUCATION,
        PLAYER_ZERO_WIN_BY_TIMEOUT,
        PLAYER_ONE_WIN_BY_TIMEOUT,
        DRAW_BY_HALF_MOVES_LIMIT,
        DRAW_BY_STALMATE,
		DRAW_BY_REPETITIONS,
        DRAW_BY_ADJUCATION
    };

    /* Before seting up a match between given engines,
    *  we also mix their sides.
    */
    template <bool EnableLog>
    PlayerPerspectiveResult mixedMatch(GameSpecPacket& packet);
private:
    template <bool EnableLog>
    void sentPosition(const std::string& start_fen, 
                      const FullInfoRecord& record,
                      EnginePlayer player,
                      enumLogLabel ret_msg_label);
    
    template <bool EnableLog>
    Move32b getPlayerMove(SearchLimits limits, 
                          Position& pos,
                          EnginePlayer player,
                          Score& score,
                          enumLogLabel ret_msg_label);

    PlayerPerspectiveResult resultToPerspectiveResult(Game::Result result, bool zero_player_white);

    static constexpr int _LowScore = 90;
    static constexpr int _AdjucateMoveLimit = 35;
};

bool isZeroPlayerWin(SelfGame::PlayerPerspectiveResult result);
bool isOnePlayerWin(SelfGame::PlayerPerspectiveResult result);
bool isDraw(SelfGame::PlayerPerspectiveResult result);

} // namespace Utils
