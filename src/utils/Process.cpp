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
#include "backend/PackedNetwork.hpp"
#include "backend/Tablebase.hpp"

namespace utils {

#if defined(_MSC_VER)
EngineProcess::~EngineProcess() {
    if (hthread) CloseHandle(hthread);
    if (hproc) {
        CloseHandle(hproc);
        TerminateProcess(hproc, 0);
    }
}

void EngineProcess::initProc(EngineProcess& proc) {
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

    FILE* fin = _fdopen(fd_in, "w");
    FILE* fout = _fdopen(fd_out, "r");

    proc.hproc = pi.hProcess;
    proc.hthread = pi.hThread;
    proc.proc_stdin = std::make_unique<std::ofstream>(fin);
    proc.proc_stdout = std::make_unique<std::ifstream>(fout);
}

void EngineProcess::waitForProcess() {
    WaitForSingleObject(hproc, INFINITE);
}

bool EngineProcess::isAlive() const {
    if (!hproc) 
        return false;

    DWORD status;
    if (GetExitCodeProcess(hproc, &status))
        return status == STILL_ACTIVE;

    return false;
}

#else // !defined(_MSC_VER)
EngineProcess::~EngineProcess() {
    if (isAlive()) kill(pid, SIGTERM);
}

void EngineProcess::initProc(EngineProcess& proc) {
    int out_pipe[2];
    int in_pipe[2];

    if (pipe(out_pipe) == -1 or pipe(in_pipe) == -1) {
        FAILED("Failed to create pipes");
    }

    const pid_t pid = fork();

    if (pid == 0) {
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
                  static_cast<char*>(nullptr)) < 0)
            return;
    }
    else {
        close(in_pipe[0]);
        close(out_pipe[1]);

        proc.pid = pid;
        proc.in_buf = std::make_unique<EngineProcess::filebuf>(in_pipe[1], std::ios::out);
        proc.proc_stdin = std::make_unique<std::ostream>(proc.in_buf.get());
        proc.out_buf = std::make_unique<EngineProcess::filebuf>(out_pipe[0], std::ios::in);
        proc.proc_stdout = std::make_unique<std::istream>(proc.out_buf.get());
    }
}

void EngineProcess::waitForProcess() {
    if (pid > 0)
        waitpid(pid, nullptr, 0);
}

bool EngineProcess::isAlive() const {
    if (!pid)
        return false;

    if (kill(pid, 0) < 0)
        return false;

    return true;
}
#endif // _MSC_VER

void EngineProcess::syncUntilReady(enumLogLabel thread_label) {
    log(*proc_stdin, "isready");

    for (std::string line; readline(*proc_stdout, line) and line != "readyok"; ) {
        labelLog(std::cout, LOG_DEBUG | LOG_ENGINE_0 | thread_label, line);
    }
}

} // namespace utils
