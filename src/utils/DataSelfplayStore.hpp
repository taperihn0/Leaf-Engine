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

/* Single selfplay data stream: one file per game outcome and per worker thread.
*/
struct Label {
    Label() = default;
    Label(Game::Result game_result, uint thread_id)
        : result(canonicalResult(game_result))
        , thr_id(thread_id) {}

    /* 'Game::Result' has many win/draw variants, but selfplay data is stored
    *  in three buckets. Normalizing when the label is created keeps hashing
    *  and equality consistent, so e.g. a mate win and a timeout win are
    *  appended to the same file.
    */
    _NODISCARD static Game::Result canonicalResult(Game::Result game_result) {
        if (isWhiteWin(game_result))
            return Game::WHITE_WIN_BY_MATE;
        else if (isBlackWin(game_result))
            return Game::BLACK_WIN_BY_MATE;
        else if (isDraw(game_result))
            return Game::DRAW_BY_STALMATE;

        return Game::GAME_INVALID;
    }

    Game::Result result = Game::GAME_INVALID;
    uint         thr_id = 0;

    bool operator==(const Label&) const = default;
};

} // namespace utils::db::internal

template <>
struct std::hash<utils::db::internal::Label> {
    size_t operator()(const utils::db::internal::Label& l) const noexcept {
        const size_t h1 = isWhiteWin(l.result) ? 0 : 
                          isBlackWin(l.result) ? 1 :
                                                 2;
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

/* Writes selfplay training entries to files inside a single session directory.
*  Every (outcome, thread) pair owns its own file, so threads can write
*  concurrently as long as they use their own thread id in the label.
*/
class Session {
public:
    using Label = internal::Label;

    Session() = delete;
    Session(size_t id, std::filesystem::path fp);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    _NODISCARD _INLINE size_t id() const { return _id; }
    _NODISCARD _INLINE const std::filesystem::path& path() const { return _fp; }

    /* Thread-safe for concurrent calls with distinct labels. Returns false when
    *  the label has no matching file or the entry could not be written.
    */
    _NODISCARD bool write(const TrainingDataEntry& entry, Label label) const;
private:
    void createWorkspace();

    size_t                  _id;
    std::filesystem::path   _fp;
    std::unordered_map<Label, std::unique_ptr<std::ofstream>>   
                                         _thr_files;
};

/* A single selfplay run:
*      <base dir>/selfplay_<month><day>_<key>/session<N>/
*  The run directory is created lazily on the first session request, so one
*  instance represents one selfplay run.
*/
class SelfplayInstance {
public:
    SelfplayInstance();
    explicit SelfplayInstance(std::filesystem::path base_dir);

    SelfplayInstance(const SelfplayInstance&) = delete;
    SelfplayInstance& operator=(const SelfplayInstance&) = delete;

    /* Creates the run workspace on first use, then a new session in it.
    *  Thread-safe. Returned reference stays valid for the lifetime of the
    *  instance.
    */
    Session& querySessionHandler();

    _NODISCARD _INLINE const std::filesystem::path& tournamentDirectory() const { 
        return _tournament_fp; 
    }
private:
    void createTournamentWorkspace();
    _NODISCARD static std::filesystem::path makeTournamentDirectoryPath();

    std::filesystem::path                 _base_fp;
    std::filesystem::path                 _tournament_fp;
    std::vector<std::unique_ptr<Session>> _sessions;
    mutable std::mutex                    _mtx;
    bool                                  _workspace_created = false;
};

/* The whole selfplay database, rooted at 'data/selfplay' by default. It owns
*  the bulk operations over collected data:
*      toBulletFormat() - rewrites raw sessions in place,
*      shuffleData()    - merges and shuffles bullet sessions for training.
*/
class SelfplayDb {
public:
    SelfplayDb();
    explicit SelfplayDb(std::filesystem::path root);

    _NODISCARD _INLINE const std::filesystem::path& root() const { return _root; }

    /* Every directory under the root that stores selfplay '.tdf' files,
    *  sorted for deterministic output.
    */
    _NODISCARD std::vector<std::filesystem::path> sessionDirectories() const;

    /* Rewrites every session in place to bullet format. Sessions already
    *  marked as converted in their 'session.info' are skipped.
    *  Returns false if any session could not be converted.
    */
    bool toBulletFormat();

    /* Merges and shuffles the bullet data of every session into
    *  '<root>_shuffled/<relative session dir>.tdf'. Sessions that were not
    *  converted to bullet format are reported and skipped.
    */
    bool shuffleData();
private:
    std::filesystem::path _root;
};

} // namespace utils::db
