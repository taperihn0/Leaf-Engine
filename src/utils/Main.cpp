#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "Units.hpp"

#include <sstream>

int main(int argc, char* argv[]) {
	ZobristHash::fillKeys();
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();

	Search search(TranspositionTable(1_MB));

	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Polish Chess Engine, " << EngineName << " by " << Author << '\n';
	std::cout << "IN UTILS MODE" << std::endl;

    std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

		if (token == "test_packed_pos")		   Utils::packedPositionTests();
		else if (token == "test_ccr_one_hour") Utils::ccrOneHourTest(search);
		else if (token == "test_see")		   Utils::seeTests();
		else if (token == "test_all")		   Utils::runTests(search);

	} while (command != "quit");
}
