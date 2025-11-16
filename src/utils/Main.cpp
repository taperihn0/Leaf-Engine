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
    std::string filepath;
    strm >> std::skipws >> filepath;

    std::ifstream input(filepath);

    if (!input) {
        std::cout << "Failed to open file: " << filepath << std::endl;
        return;
    }

    std::vector<Position> full_positions;
    std::string line;

    while (std::getline(input, line)) {
        full_positions.push_back(Position(line));
    }

    std::fstream tmp_stream("src/assets/tmp/tmp.psf", std::ios::ios_base::binary
                                                      | std::ios::ios_base::in
                                                      | std::ios::ios_base::out
                                                      | std::ios::ios_base::trunc);

    if (!tmp_stream) {
        std::cout << "Failed to open file: src/assets/tmp/tmp.psf" << std::endl;
        return;
    }

    for (auto& full_pos : full_positions) {
        Utils::PackedPosition packed = Utils::PackedPosition::packed(full_pos);
        packed.write(tmp_stream);
    }

    tmp_stream.flush();
    tmp_stream.seekp(0, std::ios::beg);

    std::vector<Utils::PackedPosition> packed_positions = Utils::PackedPosition::fullRead(tmp_stream);

    ASSERT(packed_positions.size() == full_positions.size(), 
        "Position number does not match: "
        + std::to_string(packed_positions.size()) + " != "
        + std::to_string(full_positions.size()));

    for (size_t i = 0; i < packed_positions.size(); i++) {
        Position unpack = Utils::PackedPosition::unpacked(packed_positions[i]);

        if (unpack != full_positions[i]) {
            unpack.print();
            full_positions[i].print();
            std::cout << "Position number " << i << " does not match" << std::endl;
            return;
        }
    }

    std::cout << "Successfully packed all positions" << std::endl;
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
        else if (token == "test_packed_file")  parsePackedFile(strm);

	} while (command != "quit");
}
