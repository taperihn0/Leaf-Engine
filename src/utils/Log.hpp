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

    bool open(const std::filesystem::path& filepath, 
              bool also_stdout = false, 
              std::ios::openmode mode = std::ios::out | std::ios::app);
    void close();
    void setStream(std::ostream& os);
    void setAlsoStdout(bool enable);
    bool isOpen() const;
    void flush();

    void write(uint32_t label, const std::string& message);
    void write(const std::string& message);
    void info(const std::string& message, uint32_t extra_label = LOG_NO_LABEL);
    void debug(const std::string& message, uint32_t extra_label = LOG_NO_LABEL);

    template <typename Arg1, typename Arg2, typename... Args>
    void write(uint32_t label, const Arg1& a1, const Arg2& a2, const Args&... rest);

    template <typename Arg1, typename Arg2, typename... Args>
    void write(const Arg1& a1, const Arg2& a2, const Args&... rest);

    static Log& get();
    static void setDefaultFile(const std::filesystem::path& filepath, 
                               bool also_stdout = false, 
                               std::ios::openmode mode = std::ios::out | std::ios::app);

    static void log(uint32_t label, const std::string& message);
    static void log(const std::string& message);
    static void sLog(uint32_t label, const std::string& message);
    static void sLog(const std::string& message);
    static void infoDefault(const std::string& message, uint32_t extra_label = LOG_NO_LABEL);
    static void debugDefault(const std::string& message, uint32_t extra_label = LOG_NO_LABEL);

    template <typename Arg1, typename Arg2, typename... Args>
    static void sLog(uint32_t label, const Arg1& a1, const Arg2& a2, const Args&... rest);

    template <typename Arg1, typename Arg2, typename... Args>
    static void sLog(const Arg1& a1, const Arg2& a2, const Args&... rest);

    static std::string formatLabel(uint32_t label);
private:
    template <typename... Args>
    static std::string concatArgs(const Args&... args);

    std::ostream* _os = &std::cout;
    std::unique_ptr<std::ofstream> _file_stream = nullptr;
    bool _also_stdout = false;
};

template <typename... Args>
_INLINE std::string Log::concatArgs(const Args&... args) {
    std::stringstream ss;
    (ss << ... << args);
    return ss.str();
}

template <typename Arg1, typename Arg2, typename... Args>
void Log::write(uint32_t label, const Arg1& a1, const Arg2& a2, const Args&... rest) {
    write(label, concatArgs(a1, a2, rest...));
}

template <typename Arg1, typename Arg2, typename... Args>
void Log::write(const Arg1& a1, const Arg2& a2, const Args&... rest) {
    write(LOG_NO_LABEL, concatArgs(a1, a2, rest...));
}

template <typename Arg1, typename Arg2, typename... Args>
void Log::sLog(uint32_t label, const Arg1& a1, const Arg2& a2, const Args&... rest) {
    get().write(label, concatArgs(a1, a2, rest...));
}

template <typename Arg1, typename Arg2, typename... Args>
void Log::sLog(const Arg1& a1, const Arg2& a2, const Args&... rest) {
    get().write(LOG_NO_LABEL, concatArgs(a1, a2, rest...));
}

} // namespace utils
