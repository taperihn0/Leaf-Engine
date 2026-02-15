#include "PackedPosition.hpp"
#include "backend/Common.hpp"
#include "frontend/UCI.hpp"
#include "Units.hpp"
#include "Collector.hpp"
#include "Opening.hpp"
#include "Filepath.hpp"
#include "PostProcess.hpp"
#include "SPSA.h"
#include "frontend/Setup.hpp"

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

    std::vector<Utils::ExtPackedPosition> pack_positions = Utils::ExtPackedPosition::fullRead(input);
    size_t n = pack_positions.size();

    if (begin < 0 or begin + count > n) {
        std::cout << "Invalid access indexes" << std::endl;
        return;
    }

    for (size_t i = begin; i < begin + count; i++) {
        Position unpack = Utils::ExtPackedPosition::unpacked(pack_positions[i]);
        unpack.print();
    }

    std::cout.flush();
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

void parseFilterTrainData(std::istringstream& strm) {
    std::string from_path;
    std::string to_path;

    strm >> std::skipws >> from_path >> std::skipws >> to_path;

    std::ifstream input(from_path, std::ios_base::binary);

    if (!input) {
        ASSERT(false, "Failed to open file: " + from_path);
        return;
    }

    std::ofstream output(to_path, std::ios_base::binary);

    if (!output) {
        ASSERT(false, "Failed to open file: " + to_path);
        return;
    }

    Utils::TrainingDataEntry entry;

    auto pos_filter = [](const Utils::PackedPosition& pack) -> bool {
        Position pos = Utils::PackedPosition::unpacked(pack);

        return !pos.isInCheck(pos.getTurn()) and pos.isQuiet();
    };

    while (Utils::TrainingDataEntry::read(input, entry)) {
        if (pos_filter(entry.getPosition())) {
            if (!Utils::TrainingDataEntry::write(output, entry)) {
                ASSERT(false, "Failed to write to file: " + to_path);
                return;
            }
        }
    }
}

void parseSPSA(std::istringstream& strm) {
    Utils::SPSA_Tuning tuner;
    tuner.start();
}

int main(int argc, char* argv[]) {
	setupInternals();

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
        else if (token == "test_ccr_one_hour")     Utils::ccrOneHourTest();
        else if (token == "test_see")		       Utils::seeTests();
        else if (token == "test_pack_on")          Utils::parsePackedFile(strm);
        else if (token == "test_extpack_on")       Utils::parseExtPackedFile(strm);
        else if (token == "self_play")		       parseSelfPlay(collector, strm);
        else if (token == "load_openings")         Utils::OpeningGenerator::load();
        else if (token == "view_positions")        parseShowPositions(strm);
        else if (token == "merge_selfplay_files")  parseMerge(strm);
        else if (token == "packed_to_train_entry") parse2TrainEntry(strm);
        else if (token == "filter_train_data")     parseFilterTrainData(strm);
        else if (token == "spsa")                  parseSPSA(strm);

	} while (command != "quit");
}
