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
#include "Log.hpp"
#include "Paths.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <execution>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace utils::db {

namespace {

constexpr std::string_view SessionInfoFileName = "session.info";
constexpr std::string_view TempFileSuffix      = ".bullet_tmp";
constexpr std::string_view BackupFileSuffix    = ".raw_backup";

/* 'TrainingDataEntry' is larger in memory than on disk, but both the raw and
*  the bullet record are packed into the same fixed size.
*/
constexpr size_t TrainingRecordSize = sizeof(BulletChessBoard);
static_assert(TrainingRecordSize == 32, "Unexpected on-disk training record size");

bool collectTdfFiles(const std::filesystem::path& dir, 
                     std::vector<std::filesystem::path>& files) 
{
    std::error_code err;
    std::filesystem::directory_iterator it(dir, err), end;

    for (; it != end and !err; it.increment(err)) {
        if (!it->is_regular_file(err))
            continue;

        if (it->path().extension() == ".tdf")
            files.push_back(it->path());
    }

    return !err;
}

std::filesystem::path tempPathFor(const std::filesystem::path& fp) {
    return std::filesystem::path(fp.string() + std::string(TempFileSuffix));
}

std::filesystem::path backupPathFor(const std::filesystem::path& fp) {
    return std::filesystem::path(fp.string() + std::string(BackupFileSuffix));
}

std::filesystem::path shuffledRootFor(const std::filesystem::path& root) {
    std::filesystem::path shuffled_root = root;
    std::string name = shuffled_root.filename().string();

    /* '.', '..' and '/' have no usable directory name to derive from. */
    if (name.empty() or name == "." or name == "..")
        name = "selfplay";

    shuffled_root.replace_filename(name + "_shuffled");
    return shuffled_root;
}

/* Directories holding the shuffled output of a database. They are written in
*  bullet format without a 'session.info', so they must never be picked up as
*  a session by a later pass.
*/
bool isShuffledOutputDir(const std::filesystem::path& dir, 
                         const std::filesystem::path& root) {
    static constexpr std::string_view Suffix = "_shuffled";

    const std::filesystem::path relative = dir.lexically_relative(root);

    for (const auto& part : relative) {
        const std::string name = part.string();

        if (name.size() > Suffix.size() and name.ends_with(Suffix))
            return true;
    }

    return false;
}

bool replaceFile(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::error_code err;
    std::filesystem::rename(from, to, err);

    if (!err)
        return true;

    /* Some platforms and filesystems refuse to rename over an existing file.
    *  Overwriting through a copy keeps 'to' intact if the copy fails, unlike
    *  removing it and retrying the rename.
    */
    err.clear();
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, err);

    if (err)
        return false;

    err.clear();
    std::filesystem::remove(from, err);
    return !err;
}

/* Swaps the converted temporaries in, moving the raw originals aside first.
*  A failure at any point restores the session to its previous state, so a
*  session can never end up half converted (which the next run would then
*  convert a second time and corrupt).
*/
bool commitConvertedFiles(
    const std::vector<std::pair<std::filesystem::path, std::filesystem::path>>& pending) 
{
    std::error_code err;

    size_t moved = 0;

    for (; moved < pending.size(); moved++) {
        const std::filesystem::path& fp = pending[moved].first;

        if (!replaceFile(fp, backupPathFor(fp))) {
            lg::warning("Failed to move aside raw file: ", fp.string());
            break;
        }
    }

    const auto rollback = [&pending, &err, &moved]() {
        for (size_t i = 0; i < moved; i++) {
            const std::filesystem::path& fp = pending[i].first;
            std::error_code restore_err;

            /* Drop the installed bullet copy, if it was already written. */
            std::filesystem::remove(fp, restore_err);
            std::filesystem::rename(backupPathFor(fp), fp, restore_err);
        }

        for (const auto& [fp, tmp_fp] : pending)
            std::filesystem::remove(tmp_fp, err);
    };

    if (moved != pending.size()) {
        rollback();
        return false;
    }

    size_t installed = 0;

    for (; installed < pending.size(); installed++) {
        if (!replaceFile(pending[installed].second, pending[installed].first)) {
            lg::warning("Failed to replace '", pending[installed].first.string(), 
                        "' with its bullet-format copy.");
            break;
        }
    }

    if (installed != pending.size()) {
        /* Every original is still intact in its backup, restore all of them. */
        moved = pending.size();
        rollback();
        return false;
    }

    for (const auto& [fp, tmp_fp] : pending)
        std::filesystem::remove(backupPathFor(fp), err);

    return true;
}

/* Streams one raw file into a temporary bullet file. The temporary is left in
*  place for the caller, which only replaces the original once every file of
*  the session converted successfully.
*/
bool convertFileInPlace(const std::filesystem::path& fp,
                        std::filesystem::path& tmp_fp,
                        size_t& record_cnt) {
    record_cnt = 0;

    std::error_code err;
    const std::uintmax_t bytes = std::filesystem::file_size(fp, err);

    /* A trailing partial record would be silently dropped by the reader. */
    if (err or bytes % TrainingRecordSize != 0)
        return false;

    std::ifstream input(fp, std::ios::binary);

    if (!input)
        return false;

    tmp_fp = tempPathFor(fp);
    std::ofstream output(tmp_fp, std::ios::binary | std::ios::trunc);

    if (!output)
        return false;

    bool ok = true;

    for (TrainingDataEntry entry; ok and TrainingDataEntry::read(input, entry); ) {
        ok = BulletChessBoard::write(output, TrainingDataEntry::toBulletFormat(entry));
        record_cnt += ok;
    }

    output.close();
    ok = ok and static_cast<bool>(output);
    ok = ok and record_cnt * TrainingRecordSize == bytes;

    if (!ok) {
        std::filesystem::remove(tmp_fp, err);
        return false;
    }

    return true;
}

bool convertSessionInPlace(const std::filesystem::path& session_dir, size_t& record_cnt) {
    record_cnt = 0;

    std::vector<std::filesystem::path> tdf_files;

    if (!collectTdfFiles(session_dir, tdf_files))
        return false;

    std::sort(tdf_files.begin(), tdf_files.end());

    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> pending;
    std::error_code err;

    for (const auto& fp : tdf_files) {
        std::filesystem::path tmp_fp;
        size_t file_records = 0;

        if (!convertFileInPlace(fp, tmp_fp, file_records)) {
            for (const auto& [other_fp, other_tmp] : pending)
                std::filesystem::remove(other_tmp, err);

            return false;
        }

        pending.emplace_back(fp, tmp_fp);
        record_cnt += file_records;
    }

    return commitConvertedFiles(pending);
}

} // namespace

SessionInfo readSessionInfo(const std::filesystem::path& session_dir) {
    SessionInfo info;

    std::ifstream input(session_dir / SessionInfoFileName);

    if (!input)
        return info;

    for (std::string key; input >> key; ) {
        if (key == "bullet_format")
            input >> info.bullet_format;
        else if (key == "records")
            input >> info.records;
        else {
            std::string ignored;
            input >> ignored;
        }
    }

    return info;
}

bool writeSessionInfo(const std::filesystem::path& session_dir, const SessionInfo& info) {
    const std::filesystem::path fp = session_dir / SessionInfoFileName;
    const std::filesystem::path tmp_fp = tempPathFor(fp);

    {
        std::ofstream output(tmp_fp, std::ios::trunc);

        if (!output)
            return false;

        output << "bullet_format " << (info.bullet_format ? 1 : 0) << '\n'
               << "records " << info.records << '\n';
        output.flush();

        if (!output)
            return false;
    }

    return replaceFile(tmp_fp, fp);
}

Session::Session(size_t id, std::filesystem::path fp)
    : _id(id)
    , _fp(std::move(fp)) {
    createSession();
}

bool Session::write(const TrainingDataEntry& entry, Label label) const {
    const auto file = _thr_files.find(label);
    return (
        file == _thr_files.end() or
        file->second == nullptr or
        !file->second->is_open()
    ) ? false : TrainingDataEntry::write(*file->second, entry);
}

void Session::createSession() {
    std::error_code err;
    std::filesystem::create_directories(_fp, err);

    if (err) {
        throw std::runtime_error(
            std::format(
                "Failed to create selfplay session directory: {} ({})", 
                _fp.string(), err.message())
        );
    }

    const auto SafeFileAdd = [this](Label label, const std::filesystem::path& filename) {
        const std::filesystem::path fp = _fp / filename;
        auto file = std::make_unique<std::ofstream>(fp, std::ios::binary | std::ios::app);

        if (!file->is_open()) {
            throw std::runtime_error(std::format("Failed to open selfplay data file: {}", fp.string()));
        }

        _thr_files.emplace(label, std::move(file));
    };

    for (uint thr_id = 0; thr_id <= static_cast<uint>(PlatformThreadLimit); thr_id++) {
        SafeFileAdd(
            Label(Game::WHITE_WIN_BY_MATE, thr_id), 
            paths::PathsManager.getWhiteWinOutputFileName(thr_id)
        );

        SafeFileAdd(
            Label(Game::BLACK_WIN_BY_MATE, thr_id), 
            paths::PathsManager.getBlackWinOutputFileName(thr_id)
        );

        SafeFileAdd(
            Label(Game::DRAW_BY_STALMATE, thr_id),  
            paths::PathsManager.getDrawOutputFileName(thr_id)
        );
    }
}

SelfplayInstance::SelfplayInstance()
    : SelfplayInstance(paths::PathsManager.getDir(paths::enumDir::DATA_SELFPLAY_DIRECTORY))
{}

SelfplayInstance::SelfplayInstance(std::filesystem::path fp)
    : _selfplay_fp(std::move(fp))
{}

std::pair<std::string, std::string> SelfplayInstance::uniqueFileLabel() {
    static constexpr std::array<std::string_view, 12> MonthNames = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec",
    };
    const auto now = std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::now()
    );
    const std::chrono::year_month_day ymd(now);

    /* Fresh key on every call, otherwise a collision on the date would be
    *  retried with the same (already existing) directory name forever.
    */
    std::string file_alpha_key(4, 'a');
    std::ranges::generate(file_alpha_key, [] -> char { 
        return static_cast<char>('a' + rnd::random<size_t>(0, 'z' - 'a')); 
    });

    return std::make_pair(
        std::format(
            "{}{}", 
            MonthNames[static_cast<uint>(ymd.month()) - 1], 
            static_cast<uint>(ymd.day())
        ),
        std::move(file_alpha_key)
    );
}

void SelfplayInstance::createTournamentWorkspace() {
    for (int attempt = 0; attempt < 1000; attempt++) {
        const auto file_label = uniqueFileLabel();
        const std::filesystem::path file = _selfplay_fp / 
            std::format("selfplay_{}_{}", file_label.first, file_label.second);

        std::error_code err;

        if (std::filesystem::create_directories(file, err)) {
            _fp = file;
            return;
        }

        /* Another run already owns this name, draw a new one. */
        if (!err)
            continue;

        throw std::runtime_error(std::format(
            "Failed to create selfplay workspace: {} ({})", 
            file.string(), err.message()));
    }

    throw std::runtime_error("Failed to find a free selfplay workspace name in: " 
                             + _selfplay_fp.string());
}

Session& SelfplayInstance::querySessionHandler() {
    std::call_once(_wrk_create, [this] { createTournamentWorkspace(); });

    const std::lock_guard<std::mutex> lock(_mtx);

    if (_sessions.size() >= static_cast<size_t>(SelfPlaySessionCountLimit)) {
        throw std::runtime_error(
            std::format("Selfplay session count limit reached in: {}", _fp.string())
        );
    }

    const size_t session_id = _sessions.size() + 1;

    const std::filesystem::path session_fp = _fp / std::format("session{}", session_id);
    _sessions.emplace_back(session_id, session_fp);

    return _sessions.back();
}

SelfplayDb::SelfplayDb()
    : SelfplayDb(paths::PathsManager.getDir(paths::enumDir::DATA_SELFPLAY_DIRECTORY))
{}

SelfplayDb::SelfplayDb(std::filesystem::path root)
    : _fp(std::move(root))
{}

std::vector<std::filesystem::path> SelfplayDb::sessionDirectories() const {
    std::vector<std::filesystem::path> sessions;
    std::error_code err;

    if (!std::filesystem::is_directory(_fp, err)) {
        return sessions;
    }

    const auto hasTrainingData = [](const std::filesystem::path& dir) {
        std::vector<std::filesystem::path> files;
        return collectTdfFiles(dir, files) and !files.empty();
    };

    /* The shuffled output of this database is already in bullet format and has
    *  no 'session.info'. Converting it again would corrupt it, so it is never
    *  treated as a session - even when the caller passes a root that contains
    *  it (e.g. 'data' instead of 'data/selfplay').
    */
    const std::filesystem::path shuffled_root = 
        shuffledRootFor(_fp).lexically_normal();

    const auto isSession = [this, &shuffled_root, &hasTrainingData]
                           (const std::filesystem::path& dir) {
        return dir.lexically_normal() != shuffled_root
               and !isShuffledOutputDir(dir, _fp)
               and hasTrainingData(dir);
    };

    if (isSession(_fp))
        sessions.push_back(_fp);

    for (std::filesystem::recursive_directory_iterator it(_fp, err), end; 
         it != end and !err; 
         it.increment(err)) 
    {
        if (it->is_directory(err) and isSession(it->path()))
            sessions.push_back(it->path());
    }

    std::sort(sessions.begin(), sessions.end());
    return sessions;
}

bool SelfplayDb::toBulletFormat() {
    const std::vector<std::filesystem::path> sessions = sessionDirectories();

    if (sessions.empty()) {
        lg::warning("No selfplay data found in '", _fp.string(), "', nothing to convert.");
        return false;
    }

    lg::info("Converting ", sessions.size(), 
             " selfplay session(s) to bullet format in place...");

    std::atomic<bool> success = true;

    std::for_each(std::execution::par, sessions.begin(), sessions.end(), 
                  [&success](const std::filesystem::path& session_dir) {
        if (readSessionInfo(session_dir).bullet_format) {
            lg::debug("Already in bullet format, skipping: ", session_dir.string());
            return;
        }

        size_t records = 0;

        if (!convertSessionInPlace(session_dir, records)) {
            lg::warning("Failed to convert session to bullet format: ", session_dir.string());
            success.store(false);
            return;
        }

        if (!writeSessionInfo(session_dir, SessionInfo{true, records})) {
            lg::warning("Failed to write '", std::string(SessionInfoFileName),
                        "' for session: ", session_dir.string());
            success.store(false);
            return;
        }

        lg::info("Serialized ", records, 
                 " record(s) to bullet format: ", session_dir.string());
    });

    if (!success.load()) {
        lg::warning("Failed to serialize all sessions to bullet format.");
        return false;
    }

    lg::info("Successfully serialized all sessions to bullet format.");
    return true;
}

bool SelfplayDb::shuffleData() {
    const std::vector<std::filesystem::path> sessions = sessionDirectories();

    if (sessions.empty()) {
        lg::warning("No selfplay data found in '", _fp.string(), "', nothing to shuffle.");
        return false;
    }

    const std::filesystem::path shuffled_root = shuffledRootFor(_fp);

    lg::info("Shuffling ", sessions.size(), " bullet-format session(s) into '", 
             shuffled_root.string(), "'...");

    std::atomic<bool> success = true;
    std::atomic<size_t> shuffled_sessions = 0;

    std::for_each(std::execution::par, sessions.begin(), sessions.end(), 
                  [this, &shuffled_root, &success, &shuffled_sessions]
                  (const std::filesystem::path& session_dir) {
        const SessionInfo info = readSessionInfo(session_dir);

        if (!info.bullet_format) {
            lg::warning("Session is not in bullet format, run 'to_bullet_format' first: ",
                        session_dir.string());
            return;
        }

        std::vector<std::filesystem::path> tdf_files;
        std::error_code err;

        if (!collectTdfFiles(session_dir, tdf_files)) {
            lg::warning("Failed to list session files: ", session_dir.string());
            success.store(false);
            return;
        }

        std::sort(tdf_files.begin(), tdf_files.end());

        std::vector<BulletChessBoard> entries;
        entries.reserve(info.records);

        for (const auto& fp : tdf_files) {
            const std::uintmax_t bytes = std::filesystem::file_size(fp, err);

            /* A trailing partial record would be dropped by the reader, which
            *  would silently shrink the merged output.
            */
            if (err or bytes % TrainingRecordSize != 0) {
                lg::warning("Corrupted training file (size not a multiple of ", 
                            TrainingRecordSize, "): ", fp.string());
                success.store(false);
                return;
            }

            std::ifstream input(fp, std::ios::binary);

            if (!input) {
                lg::warning("Failed to open input file: ", fp.string());
                success.store(false);
                return;
            }

            const size_t expected = static_cast<size_t>(bytes / TrainingRecordSize);
            size_t read_cnt = 0;

            for (BulletChessBoard entry; read_cnt < expected and BulletChessBoard::read(input, entry); ) {
                entries.push_back(entry);
                read_cnt++;
            }

            if (read_cnt != expected) {
                lg::warning("Failed to read all records of file: ", fp.string());
                success.store(false);
                return;
            }
        }

        if (entries.size() != info.records) {
            lg::warning("Session record count mismatch (session.info says ", info.records,
                        ", found ", entries.size(), "): ", session_dir.string());
        }

        /* 'lexically_relative' keeps this independent of how the root was spelled
        *  and of the current working directory. The root itself relative to the
        *  root yields '.', which has to be named after the root directory.
        */
        std::filesystem::path relative = session_dir.lexically_relative(_fp);
        std::filesystem::path target = shuffled_root;

        if (relative.empty() or relative == ".")
            target /= _fp.filename().string() + ".tdf";
        else
            target /= relative.replace_extension(".tdf");

        std::filesystem::create_directories(target.parent_path(), err);

        if (err) {
            lg::warning("Failed to create output directory: ", target.parent_path().string(),
                        " (", err.message(), ')');
            success.store(false);
            return;
        }

        std::mt19937_64 engine = rnd::getRandomEngine();
        std::shuffle(entries.begin(), entries.end(), engine);

        const std::filesystem::path tmp_target = tempPathFor(target);
        std::ofstream output(tmp_target, std::ios::binary | std::ios::trunc);

        if (!output) {
            lg::warning("Failed to open output file: ", tmp_target.string());
            success.store(false);
            return;
        }

        for (const auto& entry : entries) {
            if (!BulletChessBoard::write(output, entry)) {
                lg::warning("Failed to write entry to output: ", tmp_target.string());
                success.store(false);
                return;
            }
        }

        output.close();

        if (!output or !replaceFile(tmp_target, target)) {
            lg::warning("Failed to finalize shuffled output: ", target.string());
            std::filesystem::remove(tmp_target, err);
            success.store(false);
            return;
        }

        shuffled_sessions.fetch_add(1);
        lg::info("Shuffled ", entries.size(), " record(s): ", target.string());
    });

    if (!success.load()) {
        lg::warning("Failed to shuffle all sessions.");
        return false;
    }

    if (shuffled_sessions.load() == 0) {
        lg::warning("No session was in bullet format, nothing was shuffled.");
        return false;
    }

    lg::info("Successfully shuffled ", shuffled_sessions.load(), " session(s).");
    return true;
}

} // namespace utils::db
