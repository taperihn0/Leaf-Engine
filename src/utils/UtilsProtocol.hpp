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

#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Tests.hpp"
#include "Collector.hpp"
#include "Opening.hpp"
#include "SPSA.hpp"

namespace Utils {

/* 
*  We define a superset of UCI procotol exclusive for 
*  development purposes
*/
class UtilsProtocol : public UniversalChessInterface {
public:
    UtilsProtocol() = default;
    ~UtilsProtocol() = default;

    void loop(int argc, const char* argv[]);
private:
    void parseSelfPlay(Utils::TournamentCollector& collector, std::istringstream& strm);
    void parseShowPositions(std::istringstream& strm);
    void parseVerifySession(std::istringstream& strm);
    void parseSPSA(std::istringstream& strm);
    void parsePerft(std::istringstream& strm);
    void parseBulletFormat(std::istringstream& strm);
    
    TournamentCollector _collector;
    SPSA_Tuning         _tuner;
};

} // namespace Utils
