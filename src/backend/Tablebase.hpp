#pragma once

#include "Common.hpp"
#include "Position.hpp"

class SyzygyTablebase {
public:
    SyzygyTablebase(const SyzygyTablebase&) = delete;
    SyzygyTablebase& operator=(const SyzygyTablebase&) = delete;

    _NODISCARD static SyzygyTablebase& get();
    _NODISCARD bool loadSyzygyFile(const std::string& fp);
    _NODISCARD bool isLoaded() const;
    _NODISCARD std::string getFilePath() const;

    enum TbWdlInfo {
        WDL_LOSS,
        WDL_MAYBE_LOSS,
        WDL_DRAW,
        WDL_MAYBE_WIN,
        WDL_WIN,
        WDL_INVALID
    };

    bool probeWdl(const Position& pos, TbWdlInfo& wdl);
    bool probeDtz(const Position& pos, 
                  TbWdlInfo& wdl, 
                  uint& dtz, 
                  Move16b& move);
private:
    SyzygyTablebase() = default;
    ~SyzygyTablebase();

    bool                       _initialized = false;
    std::optional<std::string> _tb_path = std::nullopt;
};
