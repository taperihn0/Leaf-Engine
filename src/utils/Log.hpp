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
#include <sstream>
#include <string>

namespace lg {

enum class logLabel : uint32_t {
    LOG_NO_LABEL  = 0,
    LOG_DEBUG     = 1,
    LOG_INFO      = 1 << 1,
    LOG_WARNING   = 1 << 2,
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

_FORCEINLINE constexpr logLabel operator|(logLabel s0, logLabel s1) {
    return static_cast<logLabel>(static_cast<uint32_t>(s0) | static_cast<uint32_t>(s1));
}

_FORCEINLINE logLabel threadLabel(uint id) {
    ASSERT_NO_LOG(1 <= id and id <= static_cast<uint>(::utils::PlatformThreadLimit));
    return static_cast<logLabel>(static_cast<uint32_t>(logLabel::LOG_THREAD_1) << (id - 1));
}

class Log {
public:
    Log();
    explicit Log(const std::filesystem::path& filepath, 
                 std::ios::openmode mode = std::ios::out | std::ios::app);
    explicit Log(std::ostream& os);
    ~Log();

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) noexcept = default;
    Log& operator=(Log&&) noexcept = default;

    _NODISCARD std::ostream& output();
    void flush();

    void message(logLabel label, const std::string& msg);
    void warning(const std::string& msg);
    void warning(logLabel label, const std::string& msg);
    void info(const std::string& msg);
    void info(logLabel label, const std::string& msg);
    void debug(const std::string& msg);
    void debug(logLabel label, const std::string& msg);
    
    template <typename... Args>
    void message(logLabel label, Args&&... args);
    template <typename... Args>
    void message(Args&&... args);

    template <typename... Args>
    void warning(logLabel label, Args&&... args);
    template <typename... Args>
    void warning(Args&&... args);

    template <typename... Args>
    void info(logLabel label, Args&&... args);
    template <typename... Args>
    void info(Args&&... args);

    template <typename... Args>
    void debug(logLabel label, Args&&... args);
    template <typename... Args>
    void debug(Args&&... args);

    static Log& get();
private:
    void directWrite(logLabel label, std::string_view sv);
    static std::string parse(logLabel label);

    std::ostream* _os;
    std::unique_ptr<std::ofstream> _file_stream = nullptr;
};

template <typename... Args>
void Log::message(logLabel label, Args&&... args) {
    std::ostringstream ss;
    (ss << ... << std::forward<Args>(args));
    directWrite(label, ss.str());
}

template <typename... Args>
void Log::message(Args&&... args) {
    message(logLabel::LOG_NO_LABEL, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::warning(logLabel label, Args&&... args) {
    message(logLabel::LOG_WARNING | label, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::warning(Args&&... args) {
    message(logLabel::LOG_WARNING, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::info(logLabel label, Args&&... args) {
    message(logLabel::LOG_INFO | label, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::info(Args&&... args) {
    message(logLabel::LOG_INFO, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::debug(logLabel label, Args&&... args) {
    message(logLabel::LOG_DEBUG | label, std::forward<Args>(args)...);
}

template <typename... Args>
void Log::debug(Args&&... args) {
    message(logLabel::LOG_DEBUG, std::forward<Args>(args)...);
}

_INLINE void message(logLabel label, const std::string& msg) {
    Log::get().message(label, msg);
}

template <typename... Args>
_INLINE void message(logLabel label, Args&&... args) {
    Log::get().message(label, std::forward<Args>(args)...);
}

template <typename... Args>
_INLINE void message(Args&&... args) {
    Log::get().message(std::forward<Args>(args)...);
}

_INLINE void warning(const std::string& msg) {
    Log::get().warning(msg);
}

_INLINE void warning(logLabel label, const std::string& msg) {
    Log::get().warning(label, msg);
}

template <typename... Args>
_INLINE void warning(logLabel label, Args&&... args) {
    Log::get().warning(label, std::forward<Args>(args)...);
}

template <typename... Args>
_INLINE void warning(Args&&... args) {
    Log::get().warning(std::forward<Args>(args)...);
}

_INLINE void info(const std::string& msg) {
    Log::get().info(msg);
}

_INLINE void info(logLabel label, const std::string& msg) {
    Log::get().info(label, msg);
}

template <typename... Args>
_INLINE void info(logLabel label, Args&&... args) {
    Log::get().info(label, std::forward<Args>(args)...);
}

template <typename... Args>
_INLINE void info(Args&&... args) {
    Log::get().info(std::forward<Args>(args)...);
}

_INLINE void debug(const std::string& msg) {
    Log::get().debug(msg);
}

_INLINE void debug(logLabel label, const std::string& msg) {
    Log::get().debug(label, msg);
}

template <typename... Args>
_INLINE void debug(logLabel label, Args&&... args) {
    Log::get().debug(label, std::forward<Args>(args)...);
}

template <typename... Args>
_INLINE void debug(Args&&... args) {
    Log::get().debug(std::forward<Args>(args)...);
}

} // namespace lgs
