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

#if defined(_MSC_VER)
#include <io.h>
#include <fcntl.h>
#else
#include <sys/wait.h>
#include <cstdio>
#include <ext/stdio_filebuf.h>
#endif

namespace Utils {

class EngineProcess {
public:
    EngineProcess() = default;
    ~EngineProcess();

    static void initProc(EngineProcess& proc);

    void syncUntilReady(enumLogLabel thread_label);
    void waitForProcess();
    bool isAlive() const;

#if defined(_MSC_VER)
    HANDLE hproc = nullptr;
    HANDLE hthread = nullptr;
#else
    pid_t pid = 0;
private:
    using filebuf = __gnu_cxx::stdio_filebuf<char>;
    std::unique_ptr<filebuf> in_buf;
    std::unique_ptr<filebuf> out_buf;
#endif
public:
    std::unique_ptr<std::ostream> proc_stdin = nullptr;
    std::unique_ptr<std::istream> proc_stdout = nullptr;
};

} // namespace Utils
