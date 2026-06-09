/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
