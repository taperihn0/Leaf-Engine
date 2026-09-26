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

#include "Process.hpp"
#include "Log.hpp"
#include "backend/PackedNetwork.hpp"
#include "backend/Tablebase.hpp"

namespace utils {

#if defined(__GNUC__) and !defined(_WIN32)

ForkedProcess::~ForkedProcess() {
    if (isAlive()) kill(_pid, SIGTERM);
}

void ForkedProcess::init() {
    int out_pipe[2];
    int in_pipe[2];

    if (pipe(out_pipe) == -1 or pipe(in_pipe) == -1) {
        FAILED("Failed to create pipes");
    }

    const pid_t p = fork();

    if (p == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        
        close(in_pipe[1]);
        close(out_pipe[0]);

        if (ProcExecArg == "<empty>") {
            std::cout << "Unitialized process exec path" << std::endl;
            return;
        }

        // Derive settings from current process

        const std::string nn_bin_path = nn::GlobPackedNetwork.getFilePath();
        const std::string export_nn_arg = "export_net " + nn_bin_path + ' ';

        std::string syzygy_tb_arg;

        if (SyzygyTablebase::get().isLoaded()) {
            const std::string syzygy_tb_path = SyzygyTablebase::get().getFilePath();

            if (syzygy_tb_path != "<empty>")
                syzygy_tb_arg = "setoption name SyzygyPath value " + syzygy_tb_path + ' ';
        }

        if (execl(ProcExecArg.data(), 
                  ProcExecArg.data(), 
                  "--self-play",
                  export_nn_arg.c_str(), 
                  syzygy_tb_arg.c_str(),
                  nullptr) < 0)
            return;
    }
    else {
        close(in_pipe[0]);
        close(out_pipe[1]);

        _pid = p;
        _in_buf = std::make_unique<ForkedProcess::filebuf>(in_pipe[1], std::ios::out);
        _output_stream = std::make_unique<std::ostream>(_in_buf.get());
        _out_buf = std::make_unique<ForkedProcess::filebuf>(out_pipe[0], std::ios::in);
        _input_stream = std::make_unique<std::istream>(_out_buf.get());
        _log = std::make_unique<lg::Log>(*_output_stream);
    }
}

void ForkedProcess::waitForProcess() {
    if (_pid > 0)
        waitpid(_pid, nullptr, 0);
}

bool ForkedProcess::isAlive() const {
    if (!_pid)
        return false;

    if (kill(_pid, 0) < 0)
        return false;

    return true;
}

#else

EngineProcess::~EngineProcess() {
    if (_hthread) CloseHandle(_hthread);
    if (_hproc) {
        TerminateProcess(_hproc, 0);
        CloseHandle(_hproc);
    }
}

void EngineProcess::init() {
    HANDLE h_stdin_rd = nullptr;
    HANDLE h_stdin_wr = nullptr;
    HANDLE h_stdout_rd = nullptr;
    HANDLE h_stdout_wr = nullptr;

    SECURITY_ATTRIBUTES sa_attr;
    sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa_attr.bInheritHandle = TRUE;
    sa_attr.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&h_stdout_rd, &h_stdout_wr, &sa_attr, 0) or
        !CreatePipe(&h_stdin_rd, &h_stdin_wr, &sa_attr, 0))
        FAILED("Failed to create pipes");

    SetHandleInformation(h_stdout_rd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(h_stdin_wr, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));

    si.cb = sizeof(STARTUPINFO);
    si.hStdError = h_stdout_wr;
    si.hStdOutput = h_stdout_wr;
    si.hStdInput = h_stdin_rd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    char filename[512];
    const DWORD sz = GetModuleFileNameA(nullptr, filename, sizeof(filename));

    if (!sz) FAILED("Failed to get module filename");

    const std::string nn_bin_path = nn::GlobPackedNetwork.getFilePath();
    
    std::ostringstream ss;
    ss << '\"' << std::string(filename) << "\" " 
       << "--self-play "
       << "export_net " << nn_bin_path << ' ';

    if (SyzygyTablebase::get().isLoaded()) {
        const std::string syzygy_tb_path = SyzygyTablebase::get().getFilePath();

        if (syzygy_tb_path != "<empty>")
            ss << "setoption name SyzygyPath value " << syzygy_tb_path << ' ';
    }

    const std::string& cmdline = ss.str();
    std::vector<char> cmdvec(cmdline.begin(), cmdline.end());
    cmdvec.push_back('\0');

    if (!CreateProcessA(nullptr, cmdvec.data(),
                        nullptr, nullptr, TRUE, 0, nullptr, nullptr, 
                        &si, &pi)) {
        FAILED("Failed to CreateProcessA");
    }

    CloseHandle(h_stdout_wr);
    CloseHandle(h_stdin_rd);

    const int fd_in = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdin_wr), _O_WRONLY);
    const int fd_out = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdout_rd), _O_RDONLY);

    _hproc = pi._hprocess;
    _hthread = pi._hthread;

#if defined(_MSC_VER)
    FILE* fin = _fdopen(fd_in, "w");
    FILE* fout = _fdopen(fd_out, "r");

    input_stream = std::make_unique<std::ofstream>(fin);
    output_stream = std::make_unique<std::ifstream>(fout);
#elif defined(__GNUC__)
    _in_buf = std::make_unique<EngineProcess::filebuf>(fd_in, std::ios::out);
    input_stream = std::make_unique<std::ostream>(_in_buf.get());    
    _out_buf = std::make_unique<EngineProcess::filebuf>(fd_out, std::ios::in);
    output_stream = std::make_unique<std::istream>(_out_buf.get());
    _log = std::make_unique<log::Log>(output_stream);
#else
#error "Unsupported compiler for Windows"
#endif
}

void EngineProcess::waitForProcess() {
    WaitForSingleObject(_hproc, INFINITE);
}

bool EngineProcess::isAlive() const {
    if (!_hproc) 
        return false;

    DWORD status;
    if (GetExitCodeProcess(_hproc, &status))
        return status == STILL_ACTIVE;

    return false;
}

#endif

void ForkedProcess::syncUntilReady(lg::logLabel thread_label) {
    _log->message("isready");

    for (std::string line; readline(*_input_stream, line) and line != "readyok"; ) {
        _log->debug(thread_label, line);
    }
}

lg::Log& ForkedProcess::output() {
    return *_log;
}

std::istream& ForkedProcess::input() {
    return *_input_stream;
}

} // namespace utils
