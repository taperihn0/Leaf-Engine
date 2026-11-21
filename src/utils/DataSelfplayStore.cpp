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

/* 'TrainingDataEntry' is larger in memory than on disk, but both the raw and
*  the bullet record are packed into the same fixed size.
*/
constexpr size_t TrainingRecordSize = sizeof(BulletChessBoard);
static_assert(TrainingRecordSize == 32, "Unexpected on-disk training record size");

std::mutex& outputMutex() {
    static std::mutex mtx;
    return mtx;
}

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

bool replaceFile(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::error_code err;
    std::filesystem::rename(from, to, err);

    if (!err)
        return true;

    /* Windows refuses to rename over an existing file. */
    err.clear();
    std::filesystem::remove(to, err);

    if (err)
        return false;

    err.clear();
    std::filesystem::rename(from, to, err);
    return !err;
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

    for (const auto& [fp, tmp_fp] : pending) {
        if (!replaceFile(tmp_fp, fp)) {
            println("Failed to replace '", fp.string(), "' with its bullet-format copy.");

            for (const auto& [other_fp, other_tmp] : pending)
                std::filesystem::remove(other_tmp, err);

            return false;
        }
    }

    return true;
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

    static const auto SafeFileAdd = [this](Label label, const std::filesystem::path& filename) {
        const std::filesystem::path fp = _fp / filename;
        auto file = std::make_unique<std::ofstream>(fp, std::ios::binary | std::ios::app);

        if (!file->is_open()) {
            throw std::runtime_error(std::format("Failed to open selfplay data file: {}", fp.string());
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

    static constexpr auto FileAlphaKey = [] {
        std::string key(4, 0);
        std::ranges::generate(key, [] -> char { return 'a' + rnd::random<size_t>(0, 'z' - 'a' + 1); });
        return key;
    };

    return std::make_pair(
        std::format(
            "{}{}", 
            MonthNames[static_cast<uint>(ymd.month()) - 1], 
            static_cast<uint>(ymd.day())
        ),
        FileAlphaKey()
    );
}

void SelfplayInstance::createTournamentWorkspace() {
    while (true) {
        const auto file_label = uniqueFileLabel();
        const std::filesystem::path file = _selfplay_fp / 
            std::format("selfplay_{}_{}", file_label.first, file_label.second);

        if (std::filesystem::create_directories(file)) {
            _fp = file;
            break;
        }
    }
}

Session& SelfplayInstance::querySessionHandler() {
    std::call_once(_wrk_create, createTournamentWorkspace);

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

    if (hasTrainingData(_fp))
        sessions.push_back(_fp);

    for (std::filesystem::recursive_directory_iterator it(_fp, err), end; 
         it != end and !err; 
         it.increment(err)) 
    {
        if (it->is_directory(err) and hasTrainingData(it->path()))
            sessions.push_back(it->path());
    }

    std::sort(sessions.begin(), sessions.end());
    return sessions;
}

bool SelfplayDb::toBulletFormat() {
    const std::vector<std::filesystem::path> sessions = sessionDirectories();

    if (sessions.empty()) {
        println("No selfplay data found in '", _fp.string(), "', nothing to convert.");
        return false;
    }

    println("Converting ", sessions.size(), " selfplay session(s) to bullet format in place...");

    std::atomic<bool> success = true;

    std::for_each(std::execution::par, sessions.begin(), sessions.end(), 
                  [&success](const std::filesystem::path& session_dir) {
        if (readSessionInfo(session_dir).bullet_format) {
            println("Already in bullet format, skipping: ", session_dir.string());
            return;
        }

        size_t records = 0;

        if (!convertSessionInPlace(session_dir, records)) {
            println("Failed to convert session to bullet format: ", session_dir.string());
            success.store(false);
            return;
        }

        if (!writeSessionInfo(session_dir, SessionInfo{true, records})) {
            println("Failed to write '", std::string(SessionInfoFileName),
                    "' for session: ", session_dir.string());
            success.store(false);
            return;
        }

        println("Serialized ", records, " record(s) to bullet format: ", session_dir.string());
    });

    if (!success.load()) {
        println("Failed to serialize all sessions to bullet format.");
        return false;
    }

    println("Successfully serialized all sessions to bullet format.");
    return true;
}

bool SelfplayDb::shuffleData() {
    const std::vector<std::filesystem::path> sessions = sessionDirectories();

    if (sessions.empty()) {
        println("No selfplay data found in '", _fp.string(), "', nothing to shuffle.");
        return false;
    }

    std::filesystem::path shuffled_root = _fp;
    shuffled_root.replace_filename(_fp.filename().string() + "_shuffled");

    println("Shuffling ", sessions.size(), " bullet-format session(s) into '", 
            shuffled_root.string(), "'...");

    std::atomic<bool> success = true;

    std::for_each(std::execution::par, sessions.begin(), sessions.end(), 
                  [this, &shuffled_root, &success](const std::filesystem::path& session_dir) {
        const SessionInfo info = readSessionInfo(session_dir);

        if (!info.bullet_format) {
            println("Session is not in bullet format, run 'to_bullet_format' first: ",
                    session_dir.string());
            success.store(false);
            return;
        }

        std::vector<std::filesystem::path> tdf_files;
        std::error_code err;

        if (!collectTdfFiles(session_dir, tdf_files)) {
            println("Failed to list session files: ", session_dir.string());
            success.store(false);
            return;
        }

        std::sort(tdf_files.begin(), tdf_files.end());

        std::vector<BulletChessBoard> entries;
        entries.reserve(info.records);

        for (const auto& fp : tdf_files) {
            std::ifstream input(fp, std::ios::binary);

            if (!input) {
                println("Failed to open input file: ", fp.string());
                success.store(false);
                return;
            }

            for (BulletChessBoard entry; BulletChessBoard::read(input, entry); ) {
                entries.push_back(entry);
            }
        }

        const std::filesystem::path relative = std::filesystem::relative(session_dir, _fp);
        const std::filesystem::path target = 
            (shuffled_root / relative).replace_extension(".tdf");

        std::filesystem::create_directories(target.parent_path(), err);

        if (err) {
            println("Failed to create output directory: ", target.parent_path().string(),
                    " (", err.message(), ')');
            success.store(false);
            return;
        }

        std::mt19937_64 engine = rnd::getRandomEngine();
        std::shuffle(entries.begin(), entries.end(), engine);

        std::ofstream output(target, std::ios::binary | std::ios::trunc);

        if (!output) {
            println("Failed to open output file: ", target.string());
            success.store(false);
            return;
        }

        for (const auto& entry : entries) {
            if (!BulletChessBoard::write(output, entry)) {
                println("Failed to write entry to output: ", target.string());
                success.store(false);
                return;
            }
        }

        println("Shuffled ", entries.size(), " record(s): ", target.string());
    });

    if (!success.load()) {
        println("Failed to shuffle all sessions.");
        return false;
    }

    println("Successfully shuffled all sessions.");
    return true;
}

} // namespace utils::db
