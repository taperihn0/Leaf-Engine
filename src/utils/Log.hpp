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

#include <memory>

namespace utils {

enum enumLogLabel : uint32_t {
    LOG_NO_LABEL  = 0,
    LOG_DEBUG     = 1,
    LOG_INFO      = 1 << 1,
    LOG_WARNING   = 1 << 2,
    LOG_ENGINE_0  = 1 << 3,
    LOG_ENGINE_1  = 1 << 4,
    LOG_THREAD_1  = 1 << 5,
    LOG_THREAD_2  = 1 << 6,
    LOG_THREAD_3  = 1 << 7,
    LOG_THREAD_4  = 1 << 8,
    LOG_THREAD_5  = 1 << 9,
    LOG_THREAD_6  = 1 << 10,
    LOG_THREAD_7  = 1 << 11,
    LOG_THREAD_8  = 1 << 12,
    LOG_THREAD_9  = 1 << 13,
    LOG_THREAD_10 = 1 << 14,
    LOG_THREAD_11 = 1 << 15,
    LOG_THREAD_12 = 1 << 16,
    LOG_THREAD_13 = 1 << 17,
    LOG_THREAD_14 = 1 << 18,
    LOG_THREAD_15 = 1 << 19,
    LOG_THREAD_16 = 1 << 20,
};

_FORCEINLINE constexpr enumLogLabel operator|(enumLogLabel s0, enumLogLabel s1) {
    return static_cast<enumLogLabel>(static_cast<uint32_t>(s0) | static_cast<uint32_t>(s1));
}

_FORCEINLINE enumLogLabel threadLabel(uint id) {
    ASSERT_NO_LOG(1 <= id and id <= static_cast<uint>(PlatformThreadLimit));
    return static_cast<enumLogLabel>(LOG_THREAD_1 << (id - 1));
}

class Log {
public:
    Log();
    explicit Log(const std::filesystem::path& filepath, 
                 bool also_stdout = false, 
                 std::ios::openmode mode = std::ios::out | std::ios::app);
    explicit Log(std::ostream& os);
    ~Log();

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) noexcept = default;
    Log& operator=(Log&&) noexcept = default;

    void flush();

    void message(enumLogLabel label, const std::string& msg);
    
    template <typename... Args>
    void message(enumLogLabel label, Args&&... args);

    template <typename... Args>
    void message(Args&&... args);

    void warning(const std::string& msg);
    void warning(enumLogLabel label, const std::string& msg);
    void info(const std::string& msg);
    void info(enumLogLabel label, const std::string& msg);
    void debug(const std::string& msg);
    void debug(enumLogLabel label, const std::string& msg);

    static Log& get();
private:
    static std::string parse(enumLogLabel label);

    std::ostream* _os;
    std::unique_ptr<std::ofstream> _file_stream = nullptr;
};

template <typename... Args>
void Log::message(enumLogLabel label, Args&&... args) { 
    std::istringstream ss;
    (ss << ... << std::forward<Args>(args));
    message(label, ss.str());
}

template <typename... Args>
void Log::message(Args&&... args) {
    message(LOG_NO_LABEL, std::forward<Args>(args)...);
}

} // namespace utils
