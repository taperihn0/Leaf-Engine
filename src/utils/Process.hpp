#include "UtilsCommon.hpp"

#include <memory>

namespace Utils {

#if defined(_MSC_VER)

#else

#include <cstdio>
#include <ext/stdio_filebuf.h>

struct EngineProcess {
    pid_t pid;
    std::unique_ptr<std::ostream> in;
    std::unique_ptr<std::istream> out;
    using filebuf = __gnu_cxx::stdio_filebuf<char>;
    std::unique_ptr<filebuf> in_buf;
    std::unique_ptr<filebuf> out_buf;
};

EngineProcess spawnProcess();

#endif

}
