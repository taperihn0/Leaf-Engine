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
    std::unique_ptr<std::ostream> in;
    std::unique_ptr<std::istream> out;

#if defined(_MSC_VER)
#pragma WARNING("Unimplemented")
#else
    using filebuf = __gnu_cxx::stdio_filebuf<char>;
    pid_t                         pid;
    std::unique_ptr<filebuf>      in_buf;
    std::unique_ptr<filebuf>      out_buf;
#endif
};

EngineProcess spawnProcess();
void waitForProcess(EngineProcess& proc);
bool isAlive(const EngineProcess& proc);

}
