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

struct EngineProcess {
    EngineProcess() = default;
    ~EngineProcess();

    static void spawnProcess(EngineProcess& proc);
    
    void waitForProcess();
    bool isAlive() const;

#if defined(_MSC_VER)
    HANDLE hproc = nullptr;
	HANDLE hthread = nullptr;
#else
    pid_t pid = 0;
#endif
    std::unique_ptr<std::ofstream> proc_stdin = nullptr;
    std::unique_ptr<std::ifstream> proc_stdout = nullptr;
};

}
