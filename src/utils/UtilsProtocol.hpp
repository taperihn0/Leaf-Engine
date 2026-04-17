#pragma once

#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Tests.hpp"
#include "Collector.hpp"
#include "Opening.hpp"
#include "PostProcess.hpp"
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
	void parseMerge(std::istringstream& strm);
	void parse2TrainEntry(std::istringstream& strm);
	void parseVerifyTrainData(std::istringstream& strm);
	void parseSPSA(std::istringstream& strm);

	TournamentCollector _collector;
	SPSA_Tuning   _tuner;
};

} // namespace Utils
