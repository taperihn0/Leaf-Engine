#include "UtilsCommon.hpp"

#include <memory>

namespace Utils {

#if defined(_MSC_VER)

struct EngineProcess {
    static_assert(false);
};

#else

#include <cstdio>
#include <ext/stdio_filebuf.h>

struct EngineProcess {
    using filebuf = __gnu_cxx::stdio_filebuf<char>;
    pid_t                         pid;
    std::unique_ptr<std::ostream> in;
    std::unique_ptr<std::istream> out;
    std::unique_ptr<filebuf>      in_buf;
    std::unique_ptr<filebuf>      out_buf;
};

EngineProcess spawnProcess();

#endif

}
