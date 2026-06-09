#pragma once

#include "UtilsCommon.hpp"
#include "Sets.hpp"

#define _COLOR_RED         "\033[0;31m"
#define _COLOR_BRIGHT_RED  "\033[0;91m"
#define _COLOR_BRIGHT_BLUE "\033[0;94m"
#define _COLOR_GREY        "\033[0;97m"
#define _COLOR_GREEN       "\033[0;32m"
#define _COLOR_RESET       "\033[0m"

#define _TESTCASE(cmp, expc, f, ...)																		 \
{																										  \
	::Utils::_testcaseAssertion(f, expc, cmp, #cmp, #f "(" #__VA_ARGS__ ")", (int)__LINE__, __VA_ARGS__); \
}																										  \

namespace Utils {

template <typename T>
bool equal(const T& a, const T& b) {
	return a == b;
}

template <typename T>
bool nonequal(const T& a, const T& b) {
	return !(a == b);
}

template <typename T>
bool samesign(const T& a, const T& b) {
	static_assert(std::is_integral<T>::value, "samesign function handles only integer types");
	return (a < 0ll and b < 0ll) or (a > 0ll and b > 0ll) 
		   or (a == 0ll and b == 0ll);
}

_INTERNAL size_t _TestCounter = 0;

template <typename T>
using _cmp_func_t = bool(*)(const T&, const T&);

template <typename Func, typename T, typename... Args>
bool _testcaseAssertion(Func f, 
						 T expected, 
						 _cmp_func_t<T> cmp, 
						 std::string_view cmpnamestr, 
						 std::string_view fcallstr, 
						 int testline, 
						 Args&&... args) {

	T fres = f(std::forward<Args>(args)...);
	bool succes = cmp(fres, expected);
	if (!succes) std::cout << _COLOR_RED;

	std::cout << "["
		<< "TESTNUM: "    << std::setw(3) << _TestCounter
		<< ", TESTLINE: " << std::setw(3) << testline 
		<< "] $ "
		<< (succes ? _COLOR_GREEN "TESTCASE PASSED" _COLOR_RESET : _COLOR_BRIGHT_RED "TESTCASE FAILED" _COLOR_RED)
		<< ": " << fcallstr
		<< ", " << cmpnamestr << "(" << fres << ", " << expected << ") = " << (succes ? "TRUE" : "FALSE")
		<< _COLOR_RESET << std::endl;

	_TestCounter++;
	return succes;
}

_INTERNAL bool seeTests() {
	Position pos;

	enum seeTokenNum {
		FEN_NUM = 0,
		MOVE_NUM,
		EXPECTED_NUM,
	};
	
	std::cout << _COLOR_BRIGHT_BLUE "####### SEE TESTING #######\n" _COLOR_RESET;

	static auto next_token = [&](const std::string& line, size_t first) -> size_t {
		size_t last = first;
		while (last < line.size() and line[last] != ';')
			last++;
		return last;
	};

	for (uint cnt = 0; cnt < SeeTestSet.size(); cnt++) {
		const std::string fen = static_cast<std::string>(SeeTestSet.at(cnt));

		size_t ind = 0;
		Move32b move = Move32b::Null;
		int expected = 0;

		for (int i = 0; i < 3; i++) {
			size_t first = ind;
			while (fen[first] == ';' or fen[first] == ' ') first++;

			ind = next_token(fen, first);
			std::string token = fen.substr(first, ind - first);

			switch (i) {
			case FEN_NUM:
				pos.setByFEN(token);
				break;
			case MOVE_NUM:
				move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, token);
				break;
			case EXPECTED_NUM:
				expected = std::atoi(token.data());
				break;
			}
		}

		if (move.isEnPassant())
			continue;

		std::cout << "[EPD, AT " << std::setw(3) << cnt << "]: " << fen << '\n';

		 const Square dst = move.getTarget();
		 const Piece::enumType piece = move.getPiece();
		 const Piece::enumType target = pos.pieceOn(dst, pos.getOppositeTurn());

		_TESTCASE(equal, expected, _StaticExchangeEval_unittest<true>, pos, move.getOrigin(), 
				  dst, target, piece);
		_TESTCASE(samesign, expected, _StaticExchangeEval_unittest<false>, pos, move.getOrigin(),
				  dst, target, piece);
	}

	return true;
}

_INTERNAL bool ccrOneHourTest() {
	static constexpr int SearchDepth = 17;
	static_assert(1 <= SearchDepth and SearchDepth < MaxDepth);

	Position pos;
	std::string line;
	Move32b move;

	FullInfoRecord tmpgame;
	SearchLimits limits;
	limits.depth = SearchDepth;

	std::cout << _COLOR_BRIGHT_BLUE "\n####### CCR ONE HOUR STS TESTING #######\n" _COLOR_RESET;

	Search search{TranspositionTable(DefaultTTSizeMb)};

	static auto next_token = [&](const std::string& line, size_t first) -> size_t {
		size_t last = first;
		while (last < line.size() and line[last] != ';')
			last++;
		return last;
	};

	time_ms_t total_duration_ms = 0_ms;

	int lcnt = 0;
	for (const auto& sv_fen : CcrOneHourSet) {
		search.registerNewGame();

		const std::string full_fen = static_cast<std::string>(sv_fen);
		size_t next = next_token(full_fen, 0);

		std::string fen = full_fen.substr(0, next);
		pos.setByFEN(fen);
		
		std::string opt = full_fen.substr(next, full_fen.size());
		size_t ind = opt.find("bm");

		std::cout << "[EPD, LINE " << std::setw(3) << lcnt << "]: " << full_fen << '\n';

		Timer timer;
		timer.go();

		if (ind != std::string::npos) {
			ind += 3;
			size_t last = next_token(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(equal, move, Search::_findBestMove_unittest, search, pos, tmpgame, limits);
		}
		else {
			ind = opt.find("am");
			ASSERT(ind != std::string::npos, "Invalid line");
			ind += 3;
			size_t last = next_token(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(nonequal, move, Search::_findBestMove_unittest, search, pos, tmpgame, limits);
		}

		const time_ms_t duration_ms = timer.duration();
		total_duration_ms += duration_ms;

		++lcnt;
	}

	std::cout << "TEST DURATION: " << total_duration_ms << "ms" << std::endl;
	return true;
}

_INTERNAL bool nullMoveTest() {
	static constexpr int SearchDepth = 18;
	static_assert(1 <= SearchDepth and SearchDepth < MaxDepth);

	Position pos;
	std::string line;
	Move32b move;

	FullInfoRecord tmpgame;
	SearchLimits limits;
	limits.depth = SearchDepth;

	std::cout << _COLOR_BRIGHT_BLUE "\n####### NULL MOVE TESTING #######\n" _COLOR_RESET;

	Search search{TranspositionTable(DefaultTTSizeMb)};

	static auto next_token = [&](const std::string& line, size_t first) -> size_t {
		size_t last = first;
		while (last < line.size() and line[last] != ';')
			last++;
		return last;
	};

	Timer timer;
	timer.go();

	int lcnt = 0;
	for (const auto& sv_fen : NullMoveSet) {
		const std::string full_fen = static_cast<std::string>(sv_fen);
		size_t next = next_token(full_fen, 0);

		std::string fen = full_fen.substr(0, next);
		pos.setByFEN(fen);

		std::string opt = full_fen.substr(next, full_fen.size());
		size_t ind = opt.find("bm");

		std::cout << "[EPD, LINE " << std::setw(3) << lcnt << "]: " << full_fen << '\n';

		if (ind != std::string::npos) {
			ind += 3;
			size_t last = next_token(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(equal, move, Search::_findBestMove_unittest, search, pos, tmpgame, limits);
		}
		else {
			ind = opt.find("am");
			ASSERT(ind != std::string::npos, "Invalid line");
			ind += 3;
			size_t last = next_token(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(nonequal, move, Search::_findBestMove_unittest, search, pos, tmpgame, limits);
		}

		++lcnt;
	}

	time_ms_t duration_ms = timer.duration();

	std::cout << "TEST DURATION: " << duration_ms << "ms" << std::endl;
	return true;
}

_INTERNAL bool packedPositionTests() {
	std::fstream tmp_stream("src/assets/tmp/tmp.pck", 
							std::ios::in | std::ios::out | std::ios_base::binary);

	if (!tmp_stream) {
		ASSERT(false, "Failed to open file: src/assets/tmp/tmp.pck");
		return false;
	}

    static constexpr size_t PositionLimit = 400000;
	std::string line;

	for (size_t i = 0; i < getCraftyOpenings().size(); i++) {
		const std::string fen = static_cast<std::string>(getCraftyOpenings().at(i));
		
		Position pos(fen);
		std::cout << i << ": " << fen << '\n';

		ExtPackedPosition&& packed = ExtPackedPosition::packed(pos);

		if (ExtPackedPosition::unpacked(packed) != pos) {
			pos.print();
			ASSERT(false, "Failed to pack a position");
			return false;
		}

        // checking read/write
		tmp_stream.seekp(0, std::ios::beg);
		ExtPackedPosition::write(tmp_stream, packed);
		tmp_stream.flush();

		tmp_stream.seekg(0, std::ios_base::beg);
		tmp_stream.clear();

        ExtPackedPosition read_packed;
        ExtPackedPosition::read(tmp_stream, read_packed);

		if (packed != read_packed) {
            pos.print();
			ASSERT(false, "Failed to read/write a packed position");
			return false;
		}

        PackedPosition&& sfpack = PackedPosition::fromExt(packed);

        tmp_stream.seekp(0, std::ios::beg);
        PackedPosition::write(tmp_stream, sfpack);
        tmp_stream.flush();

        tmp_stream.seekg(0, std::ios_base::beg);
        tmp_stream.clear();

        ExtPackedPosition read_sfpack;
        PackedPosition::read(tmp_stream, read_sfpack);

        if (sfpack != read_sfpack) {
            pos.print();
            ASSERT(false, "Failed to read/write a binpack- packed position");
            return false;
        }
	}

	std::cout << "All tests passed" << std::endl;
	return true;
}

_INTERNAL void parseExtPackedFile(std::istringstream& strm) {
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
        Utils::ExtPackedPosition packed = Utils::ExtPackedPosition::packed(full_pos);
        Utils::ExtPackedPosition::write(tmp_stream, packed);
    }

    tmp_stream.flush();
    tmp_stream.seekg(0, std::ios::beg);

    std::vector<Utils::ExtPackedPosition> packed_positions = Utils::ExtPackedPosition::fullRead(tmp_stream);

    ASSERT(packed_positions.size() == full_positions.size(), 
           "Position number does not match: "
           + std::to_string(packed_positions.size()) + " != "
           + std::to_string(full_positions.size()));

    for (size_t i = 0; i < packed_positions.size(); i++) {
        Position unpack = Utils::ExtPackedPosition::unpacked(packed_positions[i]);

        if (Utils::ExtPackedPosition::unpacked(packed_positions[i]) != full_positions[i]) {
            unpack.print();
            full_positions[i].print();
            std::cout << "Position number " << i << " does not match" << std::endl;
            return;
        }
    }

    std::cout << "Successfully packed all positions" << std::endl;
}

_INTERNAL void parsePackedFile(std::istringstream& strm) {
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
        Utils::PackedPosition sfpack = Utils::PackedPosition::packed(full_pos);
        Utils::PackedPosition::write(tmp_stream, sfpack);
    }

    tmp_stream.flush();
    tmp_stream.seekg(0, std::ios_base::beg);

    std::vector<Utils::PackedPosition> packed_positions = Utils::PackedPosition::fullRead(tmp_stream);

    for (size_t i = 0; i < packed_positions.size(); i++) {
        if (packed_positions[i] != Utils::PackedPosition(full_positions[i])) {
            full_positions[i].print();
            std::cout << "Position number " << i << " does not match (while sf-style packing)" << std::endl;
            return;
        }
    }

    std::cout << "Successfully packed all positions" << std::endl;
}


} // namespace Utils
