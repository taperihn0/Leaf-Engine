#pragma once

#include "backend/Common.hpp"
#include "backend/Position.hpp"
#include "backend/Move.hpp"
#include "backend/Time.hpp"
#include "backend/Search.hpp"
#include "backend/Game.hpp"

#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <functional>

#if defined(DEBUG)
#define INSPECT_SELFPLAY_MATCHES
#endif

namespace Utils {

_INLINE size_t streamBytesLeft(std::istream& input) {
    auto curr_bytes = input.tellg();
    input.seekg(0, std::ios::end);
    auto end_bytes = input.tellg();
    input.seekg(curr_bytes);
    return end_bytes - curr_bytes;
}

template <typename Return, typename... Args>
Return doNothing(Args&&...) { return Return(); };

static int PlatformThreadLimit = []() -> int {
    return std::thread::hardware_concurrency();
}();

inline std::string_view ProcExecArg = "<empty>";

enum enumLogLabel : uint32_t {
    LOG_NO_LABEL  = 0,

    LOG_DEBUG     = 1,
    LOG_INFO      = 1 << 1,

    LOG_ENGINE_0  = 1 << 2,
    LOG_ENGINE_1  = 1 << 3,

    LOG_THREAD_1  = 1 << 4,
    LOG_THREAD_2  = 1 << 5,
    LOG_THREAD_3  = 1 << 6,
    LOG_THREAD_4  = 1 << 7,
    LOG_THREAD_5  = 1 << 8,
    LOG_THREAD_6  = 1 << 9,
    LOG_THREAD_7  = 1 << 10,
    LOG_THREAD_8  = 1 << 11,
    LOG_THREAD_9  = 1 << 12,
    LOG_THREAD_10 = 1 << 13,
    LOG_THREAD_11 = 1 << 14,
    LOG_THREAD_12 = 1 << 15,
    LOG_THREAD_13 = 1 << 16,
    LOG_THREAD_14 = 1 << 17,
    LOG_THREAD_15 = 1 << 18,
    LOG_THREAD_16 = 1 << 19,
};

_FORCEINLINE constexpr enumLogLabel operator|(enumLogLabel s0, enumLogLabel s1) {
    return static_cast<enumLogLabel>(static_cast<uint32_t>(s0) | 
                                     static_cast<uint32_t>(s1));
}

_FORCEINLINE enumLogLabel threadLabel(uint id) {
    ASSERTNOLOG(1 <= id && id <= static_cast<uint>(PlatformThreadLimit));
    return static_cast<enumLogLabel>(LOG_THREAD_1 << (id - 1));
}

_INLINE void labelLog(std::ostream& is, uint32_t label, const std::string& str) {
    if (label == LOG_NO_LABEL) {
        is << str << std::endl;
        return;
    }

    std::string labels;
    uint32_t working_label = label; 

    auto add_label = [&](uint32_t bit, const char* name) {
        if (working_label & bit) {
            if (!labels.empty()) labels += "|";
            labels += name;
            working_label &= ~bit;
        }
    };

    add_label(LOG_DEBUG,    "DEBUG");
    add_label(LOG_INFO,     "INFO");
    add_label(LOG_ENGINE_0, "PLAYER_0");
    add_label(LOG_ENGINE_1, "PLAYER_1");

    for (int id = 1; id <= PlatformThreadLimit; id++) {
        uint32_t bit = static_cast<uint32_t>(1) << (3 + id);
        
        if (working_label & bit) {
            if (!labels.empty()) labels += "|";
            labels += "THREAD_" + std::to_string(id);
            working_label &= ~bit;
        }
    }

    is << "[" << labels << "] " << str << std::endl;
}

_FORCEINLINE void log(std::ostream& is, const std::string& str) {
    labelLog(is, LOG_NO_LABEL, str);
}

_FORCEINLINE std::istream& readline(std::istream& os, std::string& line) {
    return std::getline(os, line);
}

static constexpr int SelfPlaySessionCountLimit = 256;

_INTERNAL std::string getWhiteWinOutputFile(int thread_num) {
    ASSERTNOLOG(thread_num <= PlatformThreadLimit);
    return "selfplay_white_win_thread_" + std::to_string(thread_num) + ".tdf";
}

_INTERNAL std::string getBlackWinOutputFile(int thread_num) {
    ASSERTNOLOG(thread_num <= PlatformThreadLimit);
    return "selfplay_black_win_thread_" + std::to_string(thread_num) + ".tdf";
}

_INTERNAL std::string getDrawOutputFile(int thread_num) {
    ASSERTNOLOG(thread_num <= PlatformThreadLimit);
    return "selfplay_draw_thread_" + std::to_string(thread_num) + ".tdf";
}

} // namespace Utils
