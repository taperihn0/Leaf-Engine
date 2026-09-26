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
#include "Log.hpp"
#include "Process.hpp"

namespace utils {

class ForkedProcess;

class PairOfForks {
public:
    PairOfForks();

    _NODISCARD std::pair<ForkedProcess&, ForkedProcess&> raw();
    _NODISCARD ForkedProcess& getFork(bool idx);
    /* Returns true when placement is swapped */
    _NODISCARD bool shuffleOrder();

    using iterator = ForkedProcess*;

    iterator begin();
    iterator end();
private:
    std::pair<ForkedProcess, ForkedProcess>   _forks;
    std::pair<ForkedProcess*, ForkedProcess*> _ordered_forks;
};

class SelfGame {
public:
    struct PositionInfo {
        Position* pos;
        sc::Score white_score;
        Move32b   moves;
    };

    struct GameSpecPacket {
        search::utils::SearchLimits limits;
        uint                        thread_id;
        Game::Result                result;
        OpeningManBase*             openings; 
        std::shared_ptr<std::vector<PositionInfo>> 
                                    data_buffer;
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

    _NODISCARD static bool isZeroPlayerWin(PlayerPerspectiveResult result);
    _NODISCARD static bool isOnePlayerWin(PlayerPerspectiveResult result);
    _NODISCARD static bool isDraw(PlayerPerspectiveResult result);

    _NODISCARD static SelfGame& get();

    /* Before seting up a match between given engines,
    *  we also mix their sides.
    */
    template <bool EnableLog>
    _NODISCARD PlayerPerspectiveResult mixedMatch(PairOfForks& competitors,
                                                  GameSpecPacket& packet,
                                                  Game* game = nullptr);
private:
    SelfGame() = default;

    template <bool EnableLog>
    void sentPosition(const std::string& start_fen, 
                      const FullInfoRecord& record,
                      ForkedProcess& player,
                      lg::logLabel thread_label);
    
    template <bool EnableLog>
    _NODISCARD Move32b getPlayerMove(search::utils::SearchLimits limits, 
                                     Position& pos,
                                     ForkedProcess& player,
                                     sc::Score& score,
                                     lg::logLabel thread_label);

    _NODISCARD PlayerPerspectiveResult 
    resultToPerspectiveResult(Game::Result result, bool zero_player_white);

    static constexpr int _LowScore = 60;
    static constexpr int _AdjucateHalfMoveLimit = 36;
};

} // namespace utils
