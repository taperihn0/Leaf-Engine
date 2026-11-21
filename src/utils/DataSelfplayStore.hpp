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
#include "TrainEntry.hpp"

#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace utils::db::internal {

struct Label {
    Label() = default;
    Label(Game::Result game_result, uint thread_id)
        : cmn_result(commonResult(game_result))
        , thr_id(thread_id) {}

    _NODISCARD static Game::Result commonResult(Game::Result game_result) {
        if (isWhiteWin(game_result))
            return Game::WHITE_WIN_BY_MATE;
        else if (isBlackWin(game_result))
            return Game::BLACK_WIN_BY_MATE;
        else if (isDraw(game_result))
            return Game::DRAW_BY_STALMATE;

        return Game::GAME_INVALID;
    }

    bool operator==(const Label&) const = default;

    Game::Result cmn_result = Game::GAME_INVALID;
    uint         thr_id = 0;
};

} // namespace utils::db::internal

template <>
struct std::hash<utils::db::internal::Label> {
    size_t operator()(const utils::db::internal::Label& l) const noexcept {
        const size_t h1 = isWhiteWin(l.cmn_result) ? 0 : 
                          isBlackWin(l.cmn_result) ? 1 : 2;
        const size_t h2 = static_cast<size_t>(l.thr_id);
        return (h2 << 2) | (h1 & 0x3);
    }
};

namespace utils::db {

struct SessionInfo {
    bool   bullet_format = false;
    size_t records       = 0;

    bool operator==(const SessionInfo&) const = default;
};

class Session {
public:
    using Label = internal::Label;

    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = default;

    explicit Session(std::filesystem::path fp);

    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = default;

    _NODISCARD static Session attached(std::filesystem::path fp);

    _NODISCARD _INLINE const std::filesystem::path& path() const { return _fp; }

    _NODISCARD bool write(const TrainingDataEntry& entry, Label label) const;

    _NODISCARD SessionInfo info() const;

    bool toBulletFormat();
    bool shuffleInto(const std::filesystem::path& target_fp) const;
private:
    Session(std::filesystem::path fp, bool create_files);
    void createSession();

    _NODISCARD SessionInfo readInfo() const;
    bool writeInfo(const SessionInfo& info) const;

    bool commitConvertedFiles(
        const std::vector<std::pair<std::filesystem::path, std::filesystem::path>>& pending,
        size_t records
    );

    std::filesystem::path   _fp;
    std::unordered_map<Label, std::unique_ptr<std::ofstream>>   
                            _thr_files;
};

class SelfplayDb;

class SelfplayInstance {
public:
    SelfplayInstance();
    explicit SelfplayInstance(std::filesystem::path fp);

    SelfplayInstance(const SelfplayInstance&) = delete;
    SelfplayInstance& operator=(const SelfplayInstance&) = delete;

    Session& querySessionHandler(SelfplayDb& db);

    _NODISCARD _INLINE const std::filesystem::path& tournamentDirectory() const { 
        return _fp; 
    }
private:
    _NODISCARD static std::pair<std::string, std::string> uniqueFileLabel();
    void createTournamentWorkspace();

    std::filesystem::path _selfplay_fp;
    std::filesystem::path _fp;
    size_t                _session_cnt = 0;
    mutable std::mutex    _mtx;
    std::once_flag        _wrk_create;
};

class SelfplayDb {
public:
    SelfplayDb();
    explicit SelfplayDb(std::filesystem::path root);

    SelfplayDb(const SelfplayDb&) = delete;
    SelfplayDb& operator=(const SelfplayDb&) = delete;

    _NODISCARD _INLINE const std::filesystem::path& root() const { return _fp; }

    _NODISCARD _INLINE const std::deque<Session>& sessions() const { return _sessions; }
    _NODISCARD _INLINE size_t size() const { return _sessions.size(); }

    Session& addSession(Session session);

    bool toBulletFormat();
    bool shuffleData();
private:
    void discoverSessions();

    std::filesystem::path _fp;
    std::deque<Session>   _sessions;
};

} // namespace utils::db
