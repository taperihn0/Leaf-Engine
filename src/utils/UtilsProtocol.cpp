#include "UtilsProtocol.hpp"

namespace Utils
{

void UtilsProtocol::parseSelfPlay(Utils::DataCollector& collector, 
                                  std::istringstream& strm) {
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

void UtilsProtocol::parseShowPositions(std::istringstream& strm) {
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

void UtilsProtocol::parseMerge(std::istringstream& strm) {
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

void UtilsProtocol::parse2TrainEntry(std::istringstream& strm) {
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

void UtilsProtocol::parseFilterTrainData(std::istringstream& strm) {
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

void UtilsProtocol::parseSPSA(std::istringstream& strm) {
    int thread_count;
    strm >> std::skipws >> thread_count;

    std::string spsa_log_path;
    strm >> std::skipws >> spsa_log_path;

    _tuner.start(thread_count, spsa_log_path);
}

void UtilsProtocol::loop(int argc, const char* argv[]) {
	std::ios_base::sync_with_stdio(false);

	std::cout << "Utility build of Leaf" << '\n';

	std::string command;

	do {
		if (!std::getline(std::cin, command))
			command = "quit";

		std::istringstream strm(command);
		std::string token;

		strm >> std::skipws >> token;

        if (token == "uci")						   parseUCI();
        else if (token == "ucinewgame") 		   parseNewGame();
        else if (token == "position")			   parsePosition(strm);
        else if (token == "print")				   _pos.print();
        else if (token == "go")					   parseGo(strm);
        else if (token == "isready")			   parseIsReady();
        else if (token == "export_net") 		   parseNet(strm);
        else if (token == "rewrite_header")		   parseRewriteNet(strm);
        else if (token == "options")			   parseShowOptions();
        else if (token == "setoption")			   parseSetOptions(strm);
        else if (token == "test_pack")             packedPositionTests();
        else if (token == "test_ccr_one_hour")     ccrOneHourTest();
        else if (token == "test_null_move")        nullMoveTest();
		else if (token == "test_see")		       seeTests();
		else if (token == "test_pack_on")          parsePackedFile(strm);
		else if (token == "test_extpack_on")       parseExtPackedFile(strm);
		else if (token == "self_play")		       parseSelfPlay(_collector, strm);
		else if (token == "load_openings")         OpeningGenerator::load();
		else if (token == "view_positions")        parseShowPositions(strm);
		else if (token == "merge_selfplay_files")  parseMerge(strm);
		else if (token == "packed_to_train_entry") parse2TrainEntry(strm);
		else if (token == "filter_train_data")     parseFilterTrainData(strm);
		else if (token == "spsa")                  parseSPSA(strm);

#if defined(_UCI_DEBUG_UTILS)
		else if (token == "see")				   parseSEE(strm);
		else if (token == "nneval")				   parseNNEval(strm);
#endif

	} while (command != "quit");
}

} // namespace Utils
