#include "SPSA.h"
#include "Process.hpp"

#include <sys/wait.h>

namespace Utils {

void SPSA_Tuning::run() {
    auto engine0 = spawnProcess();
    auto& in0 = *engine0.in;
    auto& out0 = *engine0.out;

    in0 << "uci" << std::endl;
    
    std::string response;
    while (std::getline(out0, response)) {
        std::cout << response << std::endl;
    }

    in0 << "quit" << std::endl;

    int status;
    waitpid(engine0.pid, &status, 0);

    std::cout << status << std::endl;
}

INLINE float SPSA_Tuning::match(SelfGame& judge, SearchLimits limits) {
    const auto res = judge.start(limits);
    
    return isWhiteWin(res) ?  1 :
           isBlackWin(res) ? -1 :
                              0.5;
}

}
