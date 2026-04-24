#pragma once

#include "UtilsCommon.hpp"

#include <memory>

#if defined(__GNUC__)
#include <sys/wait.h>
#include <cstdio>
#include <ext/stdio_filebuf.h>
#else
#pragma WARNING("Unimplemented")
#endif

namespace Utils {

struct EngineProcess {
    EngineProcess() = default;

    static EngineProcess spawnProcess();
    
    void waitForProcess();
    bool isAlive() const;

#if defined(_MSC_VER)
#pragma WARNING("Unimplemented")
#else
    using filebuf = __gnu_cxx::stdio_filebuf<char>;

    pid_t                         pid = 0;
    std::unique_ptr<filebuf>      in_buf = nullptr;
    std::unique_ptr<filebuf>      out_buf = nullptr;
#endif
    std::unique_ptr<std::ostream> proc_stdin = nullptr;
    std::unique_ptr<std::istream> proc_stdout = nullptr;
};

}
