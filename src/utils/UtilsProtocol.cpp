#include "UtilsProtocol.hpp"

namespace Utils
{

void UtilsProtocol::parseSelfPlay(Utils::TournamentCollector& collector, 
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
    const std::string log_dir = token;

    strm >> std::skipws >> token;
    const std::string err_log_dir = token;

    strm >> std::skipws >> token;
    SearchLimits limits = UniversalChessInterface::loadSearchLimits(strm, token);

    collector.startTournament(games_count, 
                              thread_cnt, 
                              log_dir,
                              err_log_dir, 
                              limits);
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

void UtilsProtocol::parseVerifyTrainData(std::istringstream& strm) {
    std::string fp;

    strm >> std::skipws >> fp;

    std::ifstream input(fp, std::ios_base::binary);

    if (!input) {
        ASSERT(false, "Failed to open file: " + fp);
        return;
    }

    auto pos_verify = [](const Position& pos) -> bool {
        if (!pos.isValid())
            return false;

        else if (pos.getPiecesCount() <= 6 and 
                 StaticEval::evaluateEndgame(pos) != Score::Undef)
            return false;

        else if (pos.isInCheck(pos.getTurn()))
            return false;

        return true;
    };

    Utils::TrainingDataEntry entry;

    while (Utils::TrainingDataEntry::read(input, entry)) {
        const PackedPosition pack = entry.getPosition();
        const Position pos = PackedPosition::unpacked(pack);
        
        if (!pos_verify(pos)) {
            std::cout << "Verification failed, invalid packed position: \n";

            if (pos.isValid()) pos.print();
            else pack.print();

            return;
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
		else if (token == "load_openings")         GlobOpeningGenerator.load();
		else if (token == "view_positions")        parseShowPositions(strm);
		else if (token == "verify_train_data")     parseVerifyTrainData(strm);
		else if (token == "spsa")                  parseSPSA(strm);

#if defined(_UCI_DEBUG_UTILS)
		else if (token == "see")				   parseSEE(strm);
		else if (token == "nneval")				   parseNNEval(strm);
#endif

	} while (command != "quit");
}

} // namespace Utils
