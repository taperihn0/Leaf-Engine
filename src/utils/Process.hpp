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

}
