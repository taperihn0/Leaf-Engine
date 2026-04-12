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

namespace Utils {

_INLINE size_t streamBytesLeft(std::istream& input) {
    auto curr_bytes = input.tellg();
    input.seekg(0, std::ios::end);
    auto end_bytes = input.tellg();
    input.seekg(curr_bytes);
    return end_bytes - curr_bytes;
}

static constexpr int ThreadLimit = 8;

enum enumLogLabel : uint16_t {
    LOG_NO_LABEL = 0,
    LOG_DEBUG    = 1,
    LOG_INFO     = 2,
    LOG_ENGINE_0 = 4,
    LOG_ENGINE_1 = 8,
    LOG_THREAD_1 = 16,
    LOG_THREAD_2 = 32,
    LOG_THREAD_3 = 64,
    LOG_THREAD_4 = 128,
    LOG_THREAD_5 = 256,
    LOG_THREAD_6 = 512,
    LOG_THREAD_7 = 1024,
    LOG_THREAD_8 = 2048,
};

_FORCEINLINE constexpr enumLogLabel operator|(enumLogLabel s0, enumLogLabel s1) {
    return static_cast<enumLogLabel>(static_cast<uint16_t>(s0) | static_cast<uint16_t>(s1));
}

_FORCEINLINE enumLogLabel threadLabel(uint id) {
    ASSERTNOLOG(id < ThreadLimit);
    return static_cast<enumLogLabel>(16 << id);
}

_FORCEINLINE void labelLog(std::ostream& is, uint16_t label, const std::string& str) {

#if !defined(DEBUG)
    if (label & LOG_DEBUG) 
        return;
#endif

    if (label != LOG_NO_LABEL) {
        std::string labels;

        auto add_label = [&](uint16_t bit, const char* name) {
            if (label & bit) {
                if (!labels.empty()) 
                    labels += "|";
                labels += name;
                label &= ~bit;
            }
        };

        add_label(LOG_DEBUG,    "DEBUG");
        add_label(LOG_INFO,     "INFO");
        add_label(LOG_ENGINE_0, "PLAYER_0");
        add_label(LOG_ENGINE_1, "PLAYER_1");

        const auto& thread_label = [](uint id) {
            ASSERTNOLOG(id < ThreadLimit);
            return static_cast<enumLogLabel>(16 << id);
        };

        for (uint id = 0; id < ThreadLimit; id++) {
            std::stringstream thr;
            thr << "THREAD_" << id;
            add_label(threadLabel(id), thr.str().c_str());
        }

        is << "[" << labels << "] ";
    }

    is << str << std::endl;
}

_FORCEINLINE void log(std::ostream& is, const std::string& str) {
    labelLog(is, LOG_NO_LABEL, str);
}

_FORCEINLINE std::istream& readline(std::istream& os, std::string& line) {
    return std::getline(os, line);
}

} // namespace Utils
