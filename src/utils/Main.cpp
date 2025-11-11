#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Units.hpp"
#include "Collector.hpp"

#include <sstream>

void parseSelfPlay(Utils::DataCollector& collector, std::istringstream& strm) {
	std::string token;

	strm >> std::skipws >> token;

	int games_count = std::stoi(token);

	if (games_count <= 0) {
		std::cout << "Invalid games count" << std::endl;
		return;
	}

	strm >> std::skipws >> token;
	SearchLimits limits = UniversalChessInterface::loadSearchLimits(strm, token);

	collector.startTournament(games_count, "src/utils/selfPlayData/selfplay.epd", limits);
}

int main(int argc, char* argv[]) {
	ZobristHash::fillKeys();
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	Search search(TranspositionTable(1_MB));
	Utils::DataCollector collector;

	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Utility module for " << EngineName << '\n';

    std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

			 if (token == "test_packed_pos")   Utils::packedPositionTests();
		else if (token == "test_ccr_one_hour") Utils::ccrOneHourTest(search);
		else if (token == "test_see")		   Utils::seeTests();
		else if (token == "test_all")		   Utils::runTests(search);
		else if (token == "self_play")		   parseSelfPlay(collector, strm);

	} while (command != "quit");
}
