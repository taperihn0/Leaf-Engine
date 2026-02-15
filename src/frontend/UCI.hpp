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

	void loop(int argc, const char* argv[]);

	static std::vector<OptionTunableParam>& getTunableOptions();
private:
	void parseUCI();
	void parseNewGame();
	void parsePosition(std::istringstream& strm);
	void parseGo(std::istringstream& strm);
	void parseIsReady();
	void parseNet(std::istringstream& strm);
	void parseRewriteNet(std::istringstream& strm); 
	void parseSetOptions(std::istringstream& strm);
	void parseShowOptions();

#if defined(_UCI_DEBUG_UTILS)
	void parseSEE(std::istringstream& strm);
	void parseNNEval(std::istringstream& strm);
#endif

	struct Options {
		OptionHash 					    hash;
		OptionClearHash 			    clear_hash;
		std::vector<OptionTunableParam> tunable_params;
	};

	Search         _search;
	Position 	   _pos;
	FullInfoRecord _game;
	static Options _options;
};