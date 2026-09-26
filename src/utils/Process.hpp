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
#include "Log.hpp"

#include <memory>

#if defined(__GNUC__) and !defined(_WIN32)
#include <sys/wait.h>
#include <cstdio>
#else
#include <io.h>
#include <fcntl.h>
#endif
#include <ext/stdio_filebuf.h>

namespace utils {

class ForkedProcess {
public:
    ForkedProcess() = default;
    ~ForkedProcess();

    void syncUntilReady(lg::logLabel thread_label);
    void waitForProcess();
    
    _NODISCARD bool isAlive() const;

    _NODISCARD lg::Log& output();
    _NODISCARD std::istream& input();
private:
    void init();

#if defined(_WIN32)
    HANDLE _hproc = nullptr;
    HANDLE _hthread = nullptr;
#endif
    using filebuf = __gnu_cxx::stdio_filebuf<char>;
    pid_t                         _pid = 0;
    std::unique_ptr<std::ostream> _output_stream = nullptr;
    std::unique_ptr<std::istream> _input_stream = nullptr;
    std::unique_ptr<filebuf>      _in_buf = nullptr;
    std::unique_ptr<filebuf>      _out_buf = nullptr;
    std::unique_ptr<lg::Log>      _log = nullptr;
};

} // namespace utils
