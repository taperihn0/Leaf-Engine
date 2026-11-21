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

#include "DataSelfplayStore.hpp"
#include "Paths.hpp"

namespace utils::db {

Session::Session(size_t id, std::filesystem::path fp)
    : _id(id)
    , _fp(fp)
    , _info(std::nullopt) {
    createWorkspace();
}

bool Session::write(const TrainingDataEntry& entry, Label label) {
    return TrainingDataEntry::write(*_thr_files.at(label), entry);
}

void Session::createWorkspace() {
    for (const auto resstr : { std::make_pair(Game::WHITE_WIN_BY_MATE, "white_win"), 
                               std::make_pair(Game::DRAW_BY_STALMATE, "draw"), 
                               std::make_pair(Game::BLACK_WIN_BY_MATE, "black_win") }
    ) {
        for (uint i = 0; i < PlatformThreadLimit; i++) {
            const auto filename = std::format("selfplay_{0}_thread_{1}.tdf", resstr.second, i);
            const auto label = Label{ .result = resstr.first, .thr_id = i };
            _thr_files[label] = std::make_unique<std::ofstream>((_fp / filename).string(), 
                                                                std::ios::binary | std::ios::app);
        }
    }
}

SelfplayDb::SelfplayDb()
    : _fp(paths::PathsManager.getDir(paths::DATA_SELFPLAY_DIRECTORY))
{}

Session& SelfplayDb::querySessionHandler() {
    // creates new _fp / selfplay_nov21_[3-digit random key] directory (example, date should be current day) if not already created
    // then new session in that directory and returns that
}
    
} // namespace utils::db
