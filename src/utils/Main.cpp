#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Units.hpp"
#include "Collector.hpp"
#include "Opening.hpp"

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

    int thread_cnt = std::stoi(token);

    if (thread_cnt <= 0) {
        std::cout << "Invalid thread number" << std::endl;
        return;
    }

	strm >> std::skipws >> token;
    SearchLimits limits = UniversalChessInterface::loadSearchLimits(strm, token);

	collector.startTournament(games_count, thread_cnt, limits);
}

void parsePackedFile(std::istringstream& strm) {
    std::string file;
    strm >> std::skipws >> file;

    std::ifstream input(file);

    if (!input) {
        std::cout << "Failed to open file: " << file << std::endl;
        return;
    }

    std::vector<Utils::PackedPosition> packed_positions = Utils::PackedPosition::fullRead(input);

    for (auto& packed : packed_positions) {
        Position unpack = Utils::PackedPosition::unpacked(packed);

        Position::enumStatusFlag status;

        unpack.print();

        if (!(status = unpack.isValid())) {
            unpack.print();
            ASSERT(false, "Invalid packed position: code " + toStr(status));
            return;
        }
    }
}

int main(int argc, char* argv[]) {
	ZobristHash::fillKeys();
	SlidersMagics::initAttackTables<Piece::BISHOP>();
	SlidersMagics::initAttackTables<Piece::ROOK>();
    Utils::OpeningGenerator::load();

	Search search(TranspositionTable(1_MB));
	Utils::DataCollector collector;

	// C-style streams aren't used there
	std::ios_base::sync_with_stdio(false);

	std::cout << "Utility module for " << EngineName << '\n';
    
    Position st(std::string("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR w KQkq - 0 1"));
    Utils::PackedPosition p = Utils::PackedPosition::packed(st);

    std::ofstream tmp("src/utils/selfplay/tmp.epd", std::ios::ios_base::app | std::ios::ios_base::binary);

    //if (tmp) {
    //    p.write(tmp);
    //    p.write(tmp);
    //    tmp.close();
    //}
    //else {
    //    return -1;
    //}

    std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

        if (token == "test_packed_pos")        Utils::packedPositionTests();
        else if (token == "test_ccr_one_hour") Utils::ccrOneHourTest(search);
        else if (token == "test_see")		   Utils::seeTests();
        else if (token == "test_all")		   Utils::runTests(search);
        else if (token == "self_play")		   parseSelfPlay(collector, strm);
        else if (token == "test_packed_file")  parsePackedFile(strm);

	} while (command != "quit");
}
