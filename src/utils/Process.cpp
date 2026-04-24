#include "Process.hpp"
#include "frontend/Setup.hpp"

namespace Utils {

#if defined(_MSC_VER)

EngineProcess spawnProcess() {
#pragma WARNING("Unimplemented")
	return EngineProcess{};
}

void waitForProcess(EngineProcess& proc) {
#pragma WARNING("Unimplemented")
}

#else

EngineProcess EngineProcess::spawnProcess() {
    int out_pipe[2];
    int in_pipe[2];

    if (pipe(out_pipe) == -1 or pipe(in_pipe) == -1) {
        ASSERT(false, "Failed to create pipes");
    }

    const pid_t pid = fork();

    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        
        close(in_pipe[1]);
        close(out_pipe[0]);
        
        const char* argv[] = { "LeafClone", nullptr };

        UniversalChessInterface uci;
        uci.loop(1, argv);

        _exit(0);
    }
    else {
        close(in_pipe[0]);
        close(out_pipe[1]);

        EngineProcess proc;

        proc.pid = pid;
        proc.in_buf = std::make_unique<EngineProcess::filebuf>(in_pipe[1], std::ios::out);
        proc.proc_stdin = std::make_unique<std::ostream>(proc.in_buf.get());
        proc.out_buf = std::make_unique<EngineProcess::filebuf>(out_pipe[0], std::ios::in);
        proc.proc_stdout = std::make_unique<std::istream>(proc.out_buf.get());

        return proc;
    }
}

void EngineProcess::waitForProcess() {
    waitpid(pid, nullptr, 0);
}

bool EngineProcess::isAlive() const {
    if (pid <= 0)
        return false;

    if (kill(pid, 0) < 0)
        return false;

    return true;
}

#endif

}
