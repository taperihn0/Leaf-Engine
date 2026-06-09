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

#include "backend/Position.hpp"
#include "backend/Search.hpp"
#include "backend/Game.hpp"
#include "Options.hpp"

class UniversalChessInterface {
public:
	UniversalChessInterface();
	~UniversalChessInterface() = default;

	static SearchLimits loadSearchLimits(std::istringstream& strm, std::string token);
	static std::vector<OptionTunableParam>& getTunableOptions();

	virtual void loop(int argc, const char* argv[]);
protected:
	void parseUCI();
	void parseNewGame();
	void parsePosition(std::istringstream& strm);
	void parseGo(std::istringstream& strm);
	void parseIsReady();
	bool parseNeuralNet(std::istringstream& strm);
	void parseRewriteNet(std::istringstream& strm); 
	void parseSetOptions(std::istringstream& strm);
	void parseBench(std::istringstream& strm);
	void parseShowOptions();
	void parseSelfPlay();

#if defined(_UCI_DEBUG_UTILS)
	void parseSEE(std::istringstream& strm);
	void parseNNEval(std::istringstream& strm);
#endif

	struct Options {
		OptionHash 					    hash_opt;
		OptionClearHash 			    clear_hash_opt;
		OptionPath 						syzygy_opt;
		OptionPath 						neural_net_opt;
		std::vector<OptionTunableParam> tunable_params_opt;
	};

	Search         _search;
	Position 	   _pos;
	FullInfoRecord _game;
	static Options _options;
};
