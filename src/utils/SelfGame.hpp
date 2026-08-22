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

#pragma once

#include "UtilsCommon.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace utils {

class EngineProcess;

class SelfGame {
public:
    SelfGame() = default;

    struct TrainDataSpec {
        std::vector<Position>* positions_buf;
        std::vector<sc::Score>*    white_scores_buf;
        std::vector<Move32b>*  moves_buf;
    };

    struct GameSpecPacket {
        search::utils::SearchLimits           limits;
        uint                           thread_id;
        std::shared_ptr<Game::Result>  result;
        OpeningManBase*                openings; 
        std::shared_ptr<TrainDataSpec> train_data_spec;
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
    PlayerPerspectiveResult mixedMatch(EngineProcess& engine0, 
                                       EngineProcess& engine1, 
                                       GameSpecPacket& packet);
private:
    template <bool EnableLog>
    void sentPosition(const std::string& start_fen, 
                      const FullInfoRecord& record,
                      EngineProcess& player,
                      enumLogLabel ret_msg_label);
    
    template <bool EnableLog>
    Move32b getPlayerMove(search::utils::SearchLimits limits, 
                          Position& pos,
                          EngineProcess& player,
                          sc::Score& score,
                          enumLogLabel ret_msg_label);

    PlayerPerspectiveResult resultToPerspectiveResult(Game::Result result, bool zero_player_white);

    static constexpr int _LowScore = 90;
    static constexpr int _AdjucateHalfMoveLimit = 70;
};

bool isZeroPlayerWin(SelfGame::PlayerPerspectiveResult result);
bool isOnePlayerWin(SelfGame::PlayerPerspectiveResult result);
bool isDraw(SelfGame::PlayerPerspectiveResult result);

} // namespace utils
