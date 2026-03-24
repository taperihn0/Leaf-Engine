#pragma once

#include "UtilsCommon.hpp"

namespace Utils {

class Filepath {
public:
    static INLINE std::string getWhiteWinOutputPath_asPCK(int thread_num) {
        return std::string(_FileNameWhiteWin) + "_thread_" + std::to_string(thread_num) + ".pck";
    }

    static INLINE std::string getBlackWinOutputPath_asPCK(int thread_num) {
        return std::string(_FileNameBlackWin) + "_thread_" + std::to_string(thread_num) + ".pck";
    }

    static INLINE std::string getDrawOutputPath_asPCK(int thread_num) {
        return std::string(_FileNameDraw) + "_thread_" + std::to_string(thread_num) + ".pck";
    }

    static INLINE std::string getWhiteWinOutputPath_asTDF(int thread_num) {
        return std::string(_FileNameWhiteWin) + "_thread_" + std::to_string(thread_num) + ".tdf";
    }

    static INLINE std::string getBlackWinOutputPath_asTDF(int thread_num) {
        return std::string(_FileNameBlackWin) + "_thread_" + std::to_string(thread_num) + ".tdf";
    }

    static INLINE std::string getDrawOutputPath_asTDF(int thread_num) {
        return std::string(_FileNameDraw) + "_thread_" + std::to_string(thread_num) + ".tdf";
    }
private:
    static inline constexpr std::string_view _FileNameWhiteWin = "misc/selfplay/selfplay_white_win";
    static inline constexpr std::string_view _FileNameBlackWin = "misc/selfplay/selfplay_black_win";
    static inline constexpr std::string_view _FileNameDraw     = "misc/selfplay/selfplay_draw";
};

} // namespace Utils
