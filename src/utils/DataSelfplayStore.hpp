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
*  A session without that file is still in its raw (unconverted) state.
*/
struct SessionInfo {
    bool   bullet_format = false; // '.tdf' files are stored in bullet format
    size_t records       = 0;     // training records stored in the session

    bool operator==(const SessionInfo&) const = default;
};

/* A single session directory, i.e. the '.tdf' files produced by one tournament
*  run ('session<N>' in the 'selfplay_<day>_<key>' workspace). It owns
*  everything that can be done to one session, so no bulk operation has to know
*  about the on-disk layout.
*/
class Session {
public:
    using Label = internal::Label;

    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = default;

    /* Opens a session for collecting: creates the directory and its data files. */
    explicit Session(std::filesystem::path fp);

    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = default;

    /* Attaches to a session that already exists on disk, without creating or
    *  opening anything, so read passes never modify the data they inspect.
    */
    _NODISCARD static Session attached(std::filesystem::path fp);

    _NODISCARD _INLINE const std::filesystem::path& path() const { return _fp; }

    _NODISCARD bool write(const TrainingDataEntry& entry, Label label) const;

    /* State cached in 'session.info', defaults to raw and empty when absent. */
    _NODISCARD SessionInfo info() const;

    /* Rewrites this session in place to bullet format and records the result in
    *  its 'session.info'. Either the whole session is converted or it is left
    *  untouched in its raw state, never half converted.
    */
    bool toBulletFormat();

    /* Merges and shuffles the bullet records of this session into 'target_fp'.
    *  The session must have been converted with 'toBulletFormat' first.
    */
    bool shuffleInto(const std::filesystem::path& target_fp) const;
private:
    Session(std::filesystem::path fp, bool create_files);
    void createSession();

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

    /* Creates the next session of the tournament workspace and registers it in
    *  'db', which keeps it alive for as long as the database exists. Sessions
    *  created earlier stay valid, so callers can keep the returned reference
    *  alongside references to the previous ones.
    */
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

/* A selfplay database, rooted at 'data/selfplay' by default. It discovers the
*  sessions stored under the root and keeps them alive; collecting adds new
*  sessions to it through 'SelfplayInstance::querySessionHandler'. It never
*  creates workspaces on its own.
*/
class SelfplayDb {
public:
    SelfplayDb();
    explicit SelfplayDb(std::filesystem::path root);

    SelfplayDb(const SelfplayDb&) = delete;
    SelfplayDb& operator=(const SelfplayDb&) = delete;

    _NODISCARD _INLINE const std::filesystem::path& root() const { return _fp; }

    /* Sessions under the root that directly store '.tdf' files - in the
    *  'selfplay_<day>_<key>/session<N>' layout exactly the session directories,
    *  sorted by path, followed by the ones added while collecting. The shuffled
    *  output directory of this database is excluded, as it is already in bullet
    *  format and has no 'session.info'.
    */
    _NODISCARD _INLINE const std::deque<Session>& sessions() const { return _sessions; }
    _NODISCARD _INLINE size_t size() const { return _sessions.size(); }

    /* Registers an externally created session. Adding never invalidates
    *  references to the sessions already stored.
    */
    Session& addSession(Session session);

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
    void discoverSessions();

    std::filesystem::path _fp;
    std::deque<Session>   _sessions;
};

} // namespace utils::db
