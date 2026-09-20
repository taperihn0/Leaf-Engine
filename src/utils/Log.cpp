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

#include "Log.hpp"

namespace utils {

Log::Log()
    : _os(&std::cout)
    , _file_stream(nullptr)
    , _also_stdout(false) 
{}

Log::Log(const std::filesystem::path& filepath, 
         bool also_stdout, 
         std::ios::openmode mode)
    : _os(nullptr)
    , _file_stream(nullptr)
    , _also_stdout(also_stdout) {
    open(filepath, also_stdout, mode);
}

Log::Log(std::ostream& os)
    : _os(&os), _file_stream(nullptr), _also_stdout(false) {}

Log::~Log() {
    close();
}

bool Log::open(const std::filesystem::path& filepath, 
               bool also_stdout, 
               std::ios::openmode mode) 
{
    if (filepath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(filepath.parent_path(), ec);
    }

    _file_stream = std::make_unique<std::ofstream>(filepath, mode);
    _also_stdout = also_stdout;

    if (_file_stream->is_open()) {
        _os = _file_stream.get();
        return true;
    }

    _file_stream = nullptr;
    _os = &std::cout;
    return false;
}

void Log::close() {
    if (_file_stream && _file_stream->is_open()) {
        _file_stream->flush();
        _file_stream->close();
    }

    _file_stream = nullptr;
    _os = &std::cout;
    _also_stdout = false;
}

void Log::setStream(std::ostream& os) {
    if (_file_stream && _file_stream->is_open()) {
        _file_stream->flush();
        _file_stream->close();
        _file_stream = nullptr;
    }
    _os = &os;
}

void Log::setAlsoStdout(bool enable) {
    _also_stdout = enable;
}

bool Log::isOpen() const {
    return _file_stream != nullptr ? _file_stream->is_open() : (_os != nullptr);
}

void Log::flush() {
    if (_os) _os->flush();
    if (_also_stdout && _os != &std::cout)
        std::cout.flush();
}

std::string Log::formatLabel(uint32_t label) {
    if (label == LOG_NO_LABEL) {
        return "";
    }

    std::string labels;
    uint32_t working_label = label;

    auto add_label = [&](uint32_t bit, const char* name) {
        if (working_label & bit) {
            if (!labels.empty()) labels += "|";
            labels += name;
            working_label &= ~bit;
        }
    };

    add_label(LOG_DEBUG,    "DEBUG");
    add_label(LOG_INFO,     "INFO");
    add_label(LOG_ENGINE_0, "PLAYER_0");
    add_label(LOG_ENGINE_1, "PLAYER_1");

    const int max_threads = std::max(16, PlatformThreadLimit);

    for (int id = 1; id <= max_threads && id <= 28; ++id) {
        uint32_t bit = static_cast<uint32_t>(1) << (3 + id);
        
        if (working_label & bit) {
            if (!labels.empty()) labels += "|";
            labels += "THREAD_" + std::to_string(id);
            working_label &= ~bit;
        }
    }

    if (working_label != 0) {
        if (!labels.empty()) labels += "|";
        std::ostringstream ss;
        ss << "0x" << std::hex << working_label;
        labels += ss.str();
    }

    return "[" + labels + "] ";
}

void Log::write(uint32_t label, const std::string& message) {
    const std::string formatted = formatLabel(label) + message;

    if (_os != nullptr) {
        *_os << (formatLabel(label) + message) << std::endl;
    }

    FAILED_NO_LOG();
}

void Log::write(const std::string& message) {
    write(LOG_NO_LABEL, message);
}

void Log::info(const std::string& message, uint32_t extra_label) {
    write(LOG_INFO | extra_label, message);
}

void Log::debug(const std::string& message, uint32_t extra_label) {
    write(LOG_DEBUG | extra_label, message);
}

Log& Log::get() {
    static Log default_logger;
    return default_logger;
}

void Log::setDefaultFile(const std::filesystem::path& filepath, 
                         bool also_stdout, 
                         std::ios::openmode mode) 
{
    get().open(filepath, also_stdout, mode);
}

void Log::sLog(uint32_t label, const std::string& message) {
    get().write(label, message);
}

void Log::sLog(const std::string& message) {
    get().write(LOG_NO_LABEL, message);
}

void Log::log(uint32_t label, const std::string& message) {
    get().write(label, message);
}

void Log::log(const std::string& message) {
    get().write(LOG_NO_LABEL, message);
}

void Log::infoDefault(const std::string& message, uint32_t extra_label) {
    get().info(message, extra_label);
}

void Log::debugDefault(const std::string& message, uint32_t extra_label) {
    get().debug(message, extra_label);
}

} // namespace utils
