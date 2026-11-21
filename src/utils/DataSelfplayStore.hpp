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

#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
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

/* Cached state of one session, stored in 'session.info' next to the data
*  files. Bulk passes read it instead of probing the '.tdf' files again.
*/
struct SessionInfo {
    bool   bullet_format = false; // '.tdf' files are stored in bullet format
    size_t records       = 0;     // training records stored in the session

    bool operator==(const SessionInfo&) const = default;
};

_NODISCARD SessionInfo readSessionInfo(const std::filesystem::path& session_dir);
bool writeSessionInfo(const std::filesystem::path& session_dir, const SessionInfo& info);

class Session {
public:
    using Label = internal::Label;

    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = default;
    Session(size_t id, std::filesystem::path fp);

    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = default;

    _NODISCARD _INLINE size_t id() const { return _id; }
    _NODISCARD _INLINE const std::filesystem::path& path() const { return _fp; }

    _NODISCARD bool write(const TrainingDataEntry& entry, Label label) const;
private:
    void createSession();

    size_t                  _id;
    std::filesystem::path   _fp;
    std::unordered_map<Label, std::unique_ptr<std::ofstream>>   
                            _thr_files;
};

class SelfplayInstance {
public:
    SelfplayInstance();
    explicit SelfplayInstance(std::filesystem::path fp);

    SelfplayInstance(const SelfplayInstance&) = delete;
    SelfplayInstance& operator=(const SelfplayInstance&) = delete;

    /* Creates a new session on first use. The returned reference is invalidated
    *  by the next call, as sessions are kept in a growing vector.
    */
    Session& querySessionHandler();

    _NODISCARD _INLINE const std::filesystem::path& tournamentDirectory() const { 
        return _fp; 
    }
private:
    _NODISCARD static std::pair<std::string, std::string> uniqueFileLabel();
    void createTournamentWorkspace();

    std::filesystem::path _selfplay_fp;
    std::filesystem::path _fp;
    std::vector<Session>  _sessions;
    mutable std::mutex    _mtx;
    std::once_flag        _wrk_create;
};

/* Read-only view over a selfplay database, rooted at 'data/selfplay' by
*  default. It only ever inspects directories and files, it never creates
*  selfplay workspaces - that is what 'SelfplayInstance' is for.
*/
class SelfplayDb {
public:
    SelfplayDb();
    explicit SelfplayDb(std::filesystem::path root);

    _NODISCARD _INLINE const std::filesystem::path& root() const { return _fp; }

    /* Every directory under the root that directly stores '.tdf' files, i.e.
    *  in the 'selfplay_<day>_<key>/session<N>' layout exactly the session
    *  directories. The shuffled output directory of this database is excluded,
    *  as it is already in bullet format and has no 'session.info'.
    */
    _NODISCARD std::vector<std::filesystem::path> sessionDirectories() const;

    /* Rewrites every session in place to bullet format. Sessions already marked
    *  as converted in their 'session.info' are skipped, and a session that
    *  fails to convert is rolled back to its raw state.
    */
    bool toBulletFormat();

    /* Merges and shuffles the bullet records of every session into
    *  '<root>_shuffled/<relative session dir>.tdf'. Sessions that were not
    *  converted with 'toBulletFormat' are reported and skipped.
    */
    bool shuffleData();
private:
    std::filesystem::path _fp;
};

} // namespace utils::db
