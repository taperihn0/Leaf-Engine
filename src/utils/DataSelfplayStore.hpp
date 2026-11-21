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

#include "UtilsCommon.hpp"
#include "TrainEntry.hpp"

#include <unordered_map>

namespace utils::db::internal {

struct Label {
    Game::Result result;
    uint thr_id;
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

class Session {
public:
    using Label = internal::Label;

    Session() = delete;
    Session(size_t id, std::filesystem::path fp);

    _INLINE size_t id() const { return _id; }
    _INLINE const std::filesystem::path path() const { return _fp.value_or(std::filesystem::path("")); }

    bool write(const TrainingDataEntry& entry, Label label);
private:
    void createWorkspace();

    struct InfoFile {
        bool train_ready;
        std::filesystem::path fp;
    };

    size_t                  _id;
    std::filesystem::path   _fp;
    std::optional<InfoFile> _info; // unused for now
    std::unordered_map<Label, std::unique_ptr<std::ofstream>>   
                                         _thr_files;
};

class SelfplayDb {
public:
    SelfplayDb();

    Session& querySessionHandler();
private:
    std::filesystem::path _fp;
    std::vector<Session>  _sessions;
};

} // namespace utils::db
