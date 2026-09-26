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

namespace lg {

Log::Log()
    : _os(&std::cout)
    , _file_stream(nullptr)
{}

Log::Log(const std::filesystem::path& filepath, 
         std::ios::openmode mode)
    : _os(nullptr)
    , _file_stream(nullptr)
{
    if (filepath.has_parent_path()) {
        std::error_code ec;

        if (!std::filesystem::create_directories(filepath.parent_path(), ec)) {
            std::ostringstream ss;
            ss << "Failed to create directory of: " << filepath.parent_path().string() 
               << ", error: " << ec.value();
            throw std::runtime_error(ss.str());
        }
    }

    _file_stream = std::make_unique<std::ofstream>(filepath, mode);

    if (!_file_stream->is_open()) {
        throw std::runtime_error("Failed to open file in log: " + filepath.string());
    }

    _os = _file_stream.get();
}

Log::Log(std::ostream& os)
    : _os(&os)
    , _file_stream(nullptr)
{}

Log::~Log() {
    if (_file_stream and _file_stream->is_open()) {
        _file_stream->flush();
        _file_stream->close();
    }
}

std::ostream& Log::output() {
    if (_os != nullptr)
        return *_os;

    throw std::runtime_error("Invalid output");
}

void Log::flush() {
    if (_os != nullptr) {
        _os->flush();
    }
}

std::string Log::parse(logLabel label) {
    if (label == logLabel::LOG_NO_LABEL) {
        return "";
    }

    std::string labels;
    uint32_t working_label = static_cast<uint32_t>(label);

    auto add_label = [&](uint32_t bit, const char* name) {
        if (working_label & bit) {
            if (!labels.empty()) labels += "|";
            labels += name;
            working_label &= ~bit;
        }
    };

    add_label(static_cast<uint32_t>(logLabel::LOG_DEBUG),   "DEBUG");
    add_label(static_cast<uint32_t>(logLabel::LOG_INFO),    "INFO");
    add_label(static_cast<uint32_t>(logLabel::LOG_WARNING), "WARNING");

    const int max_threads = std::max(16, ::utils::PlatformThreadLimit);

    for (int id = 1; id <= max_threads && id <= 28; ++id) {
        uint32_t bit = static_cast<uint32_t>(logLabel::LOG_THREAD_1) << (id - 1);
        
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

void Log::message(logLabel label, const std::string& msg) {
    if (_os != nullptr) {
        *_os << parse(label) << msg << std::endl;
    }
}

void Log::warning(const std::string& msg) {
    message(logLabel::LOG_WARNING, msg);
}

void Log::warning(logLabel label, const std::string& msg) {
    message(logLabel::LOG_WARNING | label, msg);
}

void Log::info(const std::string& msg) {
    message(logLabel::LOG_INFO, msg);
}

void Log::info(logLabel label, const std::string& msg) {
    message(logLabel::LOG_INFO | label, msg);
}

void Log::debug(const std::string& msg) {
    message(logLabel::LOG_DEBUG, msg);
}

void Log::debug(logLabel label, const std::string& msg) {
    message(logLabel::LOG_DEBUG | label, msg);
}

Log& Log::get() {
    static Log DefaultLog;
    return DefaultLog;
}

} // namespace lg
