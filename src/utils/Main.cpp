#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Units.hpp"
#include "Collector.hpp"
#include "Opening.hpp"
#include "Filepath.hpp"
#include "PostProcess.hpp"

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

    std::fstream tmp_stream("src/assets/tmp/tmp.pck", std::ios::ios_base::binary
                                                    | std::ios::ios_base::in
                                                    | std::ios::ios_base::out
                                                    | std::ios::ios_base::trunc);
                                                     
    if (!tmp_stream) {
        std::cout << "Failed to open file: src/assets/tmp/tmp.pck" << std::endl;
        return;
    }

    for (auto& full_pos : full_positions) {
        Utils::PackedPosition packed = Utils::PackedPosition::packed(full_pos);
        Utils::PackedPosition::write(tmp_stream, packed);
    }

    tmp_stream.flush();
    tmp_stream.seekg(0, std::ios::beg);

    std::vector<Utils::PackedPosition> packed_positions = Utils::PackedPosition::fullRead(tmp_stream);

    ASSERT(packed_positions.size() == full_positions.size(), 
           "Position number does not match: "
           + std::to_string(packed_positions.size()) + " != "
           + std::to_string(full_positions.size()));

    for (size_t i = 0; i < packed_positions.size(); i++) {
        Position unpack = Utils::PackedPosition::unpacked(packed_positions[i]);

        if (Utils::PackedPosition::unpacked(packed_positions[i]) != full_positions[i]) {
            unpack.print();
            full_positions[i].print();
            std::cout << "Position number " << i << " does not match" << std::endl;
            return;
        }
    }

    std::cout << "Successfully packed all positions" << std::endl;
}

void parseSfPackedFile(std::istringstream& strm) {
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

    std::fstream tmp_stream("src/assets/tmp/tmp.pck", std::ios::ios_base::binary
                                                    | std::ios::ios_base::in
                                                    | std::ios::ios_base::out
                                                    | std::ios::ios_base::trunc);

    if (!tmp_stream) {
        std::cout << "Failed to open file: src/assets/tmp/tmp.pck" << std::endl;
        return;
    }

    for (auto& full_pos : full_positions) {
        Utils::SfBinFormatPosition sfpack = Utils::SfBinFormatPosition::sfPacked(full_pos);
        Utils::SfBinFormatPosition::write(tmp_stream, sfpack);
    }

    tmp_stream.flush();
    tmp_stream.seekg(0, std::ios_base::beg);

    std::vector<Utils::SfBinFormatPosition> sfpacked_positions = Utils::SfBinFormatPosition::fullRead(tmp_stream);

    for (size_t i = 0; i < sfpacked_positions.size(); i++) {
        if (sfpacked_positions[i] != Utils::SfBinFormatPosition(full_positions[i])) {
            full_positions[i].print();
            std::cout << "Position number " << i << " does not match (while sf-style packing)" << std::endl;
            return;
        }
    }

    std::cout << "Successfully packed all positions" << std::endl;
}

void parseShowPositions(std::istringstream& strm) {
    std::string filepath;
    strm >> std::skipws >> filepath;

    std::ifstream input(filepath, std::ios::ios_base::binary);

    if (!input) {
        std::cout << "Failed to open file: " << filepath << std::endl;
        return;
    }

    int begin, count;

    strm >> std::skipws >> begin >> std::skipws >> count;

    std::vector<Utils::PackedPosition> pack_positions = Utils::PackedPosition::fullRead(input);
    size_t n = pack_positions.size();

    if (begin < 0 or begin + count > n) {
        std::cout << "Invalid access indexes" << std::endl;
        return;
    }

    for (size_t i = begin; i < begin + count; i++) {
        Position unpack = Utils::PackedPosition::unpacked(pack_positions[i]);
        unpack.print();
    }
}

void parseMerge(std::istringstream& strm) {
    int collector_thread_count = 0;
    std::string merge_path;

    strm >> std::skipws >> merge_path >> std::skipws >> collector_thread_count;

    std::ofstream merge_file(merge_path, std::ios::ios_base::binary);

    if (!merge_file) {
        ASSERT(false, "Failed to open file: " + merge_path);
        return;
    }

    std::vector<std::string> input_paths;
    input_paths.reserve(collector_thread_count * 3);

    for (int thread_num = 0; thread_num < collector_thread_count; thread_num++) {
        input_paths.push_back(Utils::Filepath::getWhiteWinOutputPath_asTDF(thread_num));
        input_paths.push_back(Utils::Filepath::getBlackWinOutputPath_asTDF(thread_num));
        input_paths.push_back(Utils::Filepath::getDrawOutputPath_asTDF(thread_num));
    }

    Utils::PostProcess::mergeBinaryFiles(input_paths, merge_file);
}

void parse2TrainEntry(std::istringstream& strm) {
    int collector_thread_count = 0;
    strm >> std::skipws >> collector_thread_count;

    for (int thread_num = 0; thread_num < collector_thread_count; thread_num++) {
        std::string tdf_path = Utils::Filepath::getWhiteWinOutputPath_asTDF(thread_num);
        std::string pck_path = Utils::Filepath::getWhiteWinOutputPath_asPCK(thread_num);

        std::ifstream pck_input(pck_path, std::ios::ios_base::binary);

        if (!pck_input) {
            ASSERT(false, "Failed to open file: " + pck_path);
            return;
        }

        std::ofstream tdf_output(tdf_path, std::ios::ios_base::binary);
        
        if (!tdf_output) {
            ASSERT(false, "Failed to open file: " + tdf_path);
            return;
        }

        std::cout << "Converting file: " << pck_path << std::endl;

        bool status = 
            Utils::PostProcess::packedPos2TrainingEntryFile(pck_input, 
                                                            tdf_output, 
                                                            Utils::TrainingDataEntry::WHITE_WIN);

        if (!status) {
            ASSERT(false, "Failed to convert .pck file to .tdf file: " + tdf_path);
            return;
        }

        tdf_path = Utils::Filepath::getBlackWinOutputPath_asTDF(thread_num);
        pck_path = Utils::Filepath::getBlackWinOutputPath_asPCK(thread_num);

        pck_input.close();
        pck_input.open(pck_path, std::ios::ios_base::binary);

        if (!pck_input) {
            ASSERT(false, "Failed to open file: " + pck_path);
            return;
        }

        tdf_output.close();
        tdf_output.open(tdf_path, std::ios::ios_base::binary);

        if (!tdf_output) {
            ASSERT(false, "Failed to open file: " + tdf_path);
            return;
        }

        std::cout << "Converting file: " << pck_path << std::endl;

        status =
            Utils::PostProcess::packedPos2TrainingEntryFile(pck_input,
                                                            tdf_output,
                                                            Utils::TrainingDataEntry::BLACK_WIN);

        if (!status) {
            ASSERT(false, "Failed to convert .pck file to .tdf file: " + tdf_path);
            return;
        }

        tdf_path = Utils::Filepath::getDrawOutputPath_asTDF(thread_num);
        pck_path = Utils::Filepath::getDrawOutputPath_asPCK(thread_num);

        pck_input.close();
        pck_input.open(pck_path, std::ios::ios_base::binary);

        if (!pck_input) {
            ASSERT(false, "Failed to open file: " + pck_path);
            return;
        }

        tdf_output.close();
        tdf_output.open(tdf_path, std::ios::ios_base::binary);

        if (!tdf_output) {
            ASSERT(false, "Failed to open file: " + tdf_path);
            return;
        }

        std::cout << "Converting file: " << pck_path << std::endl;

        status =
            Utils::PostProcess::packedPos2TrainingEntryFile(pck_input,
                                                            tdf_output,
                                                            Utils::TrainingDataEntry::DRAW);

        if (!status) {
            ASSERT(false, "Failed to convert .pck file to .tdf file: " + tdf_path);
            return;
        }
    }
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

        /*
        * The performance of file operations are horrible.
        * The most important thing is they just works.
        */
             if (token == "test_pack")             Utils::packedPositionTests();
        else if (token == "load_openings")         Utils::OpeningGenerator::load();
        else if (token == "test_ccr_one_hour")     Utils::ccrOneHourTest(search);
        else if (token == "test_see")		       Utils::seeTests();
        else if (token == "test_all")		       Utils::runTests(search);
        else if (token == "self_play")		       parseSelfPlay(collector, strm);
        else if (token == "test_pack_on")          parsePackedFile(strm);
        else if (token == "test_sfpack_on")        parseSfPackedFile(strm);
        else if (token == "view_positions")        parseShowPositions(strm);
        else if (token == "merge_selfplay_files")  parseMerge(strm);
        else if (token == "packed_to_train_entry") parse2TrainEntry(strm);

	} while (command != "quit");
}
