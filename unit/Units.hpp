#pragma once

#include "../backend/Common.hpp"
#include "../backend/Position.hpp"
#include "../backend/Move.hpp"
#include "../backend/Time.hpp"
#include "../backend/Search.hpp"
#include "../backend/Game.hpp"

#include <iomanip>
#include <fstream>

#define _COLOR_RED		   "\033[0;31m"
#define _COLOR_BRIGHT_RED  "\033[0;91m"
#define _COLOR_BRIGHT_BLUE "\033[0;94m"
#define _COLOR_GREY		   "\033[0;97m"
#define _COLOR_GREEN	   "\033[0;32m"
#define _COLOR_RESET	   "\033[0m"

#define _TESTCASE(lcnt, cmp, expc, f, ...)																\
{																										\
	_testcase_assertion(f, expc, cmp, #cmp, #f "(" #__VA_ARGS__ ")", lcnt, (int)__LINE__, __VA_ARGS__); \
}																										\

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
	return (a < 0ull and b < 0ull) or (a > 0ull and b > 0ull) 
		   or (a == 0ull and b == 0ull);
}

static size_t _test_counter = 0;

template <typename T>
using _cmp_func_t = bool(*)(const T&, const T&);

template <typename Func, typename T, typename... Args>
bool _testcase_assertion(Func f, T expected, _cmp_func_t<T> cmp, std::string_view cmpnamestr, 
	std::string_view fcallstr, int testline, int fileline, Args&&... args) {
	T fres = f(std::forward<Args>(args)...);
	bool succes = cmp(fres, expected);
	if (!succes) std::cout << _COLOR_RED;
	std::cout << "["
		<< "TESTNUM: " << std::setw(3) << _test_counter
		<< ", TESTLINE: " << std::setw(3) << testline 
		<< ", FILELINE: " << std::setw(3) << fileline 
		<< "] $ "
		<< (succes ? _COLOR_GREEN "TESTCASE PASSED" _COLOR_RESET : _COLOR_BRIGHT_RED "TESTCASE FAILED" _COLOR_RED)
		<< ": " << fcallstr
		<< ", " << cmpnamestr << "(" << fres << ", " << expected << ") = " << (succes ? "TRUE" : "FALSE")
		<< _COLOR_RESET << std::endl;
	_test_counter++;
	return succes;
}

inline size_t nextToken(std::string& line, size_t first) {
	size_t last = first;
	while (last < line.size() and line[last] != ';')
		last++;
	return last;
}

namespace Units {

static bool seeTests() {
	std::ifstream file("unit/seeset.epd");
	ASSERT(file.is_open(), "Could not open file seeset.epd");

	Position pos;
	std::string line;

	enum seeTokenNum {
		FEN_NUM = 0,
		MOVE_NUM,
		EXPECTED_NUM,
	};
	
	std::cout << _COLOR_BRIGHT_BLUE "####### SEE TESTING #######\n" _COLOR_RESET;

	for (int lcnt = 0; std::getline(file, line); lcnt++) {
		size_t ind = 0;
		Move32b move = Move32b::Null;
		int expected = 0;

		for (int i = 0; i < 3; i++) {
			size_t first = ind;
			while (line[first] == ';' or line[first] == ' ') first++;

			ind = nextToken(line, first);
			std::string token = line.substr(first, ind - first);

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

		if (move.isQuiet() or move.isEnPassant())
			continue;

		std::cout << "[EPD, LINE " << std::setw(3) << lcnt << "]: " << line << '\n';

		 const Square dst = move.getTarget();
		 const Piece::enumType piece = move.getPiece();
		 const Piece::enumType target = pos.pieceOn(dst, pos.getOppositeTurn());

		_TESTCASE(lcnt, equal, expected, _StaticExchangeEval_unittest<true>, pos, move.getOrigin(), 
				  dst, target, piece);
		_TESTCASE(lcnt, samesign, expected, _StaticExchangeEval_unittest<false>, pos, move.getOrigin(),
				  dst, target, piece);

	}

	return true;
}

static bool ccrOneHourTest(Search& search) {
	std::ifstream file("unit/ccronehour.txt");
	ASSERT(file.is_open(), "Failed to open file ccronehour.epd");

	// MODIFY TO CHANGE SEARCHING DEPTH
	static constexpr int search_depth = 12;
	static_assert(1 <= search_depth and search_depth < MaxDepth);

	// MODIFY TO CHANGE NUMBER OF POSITION
	static constexpr int pos_limit = 25;
	static_assert(1 <= pos_limit and pos_limit <= 25);

	Position pos;
	std::string line;
	Move32b move;

	Game tmpgame;
	SearchLimits limits;
	limits.depth = search_depth;

	std::cout << _COLOR_BRIGHT_BLUE "\n####### CCR ONE HOUR STS TESTING #######\n" _COLOR_RESET;

	Timer timer;
	timer.go();

	for (int lcnt = 0; lcnt < pos_limit and std::getline(file, line); lcnt++) {
		size_t next = nextToken(line, 0);

		std::string fen = line.substr(0, next);
		pos.setByFEN(fen);
		
		std::string opt = line.substr(next, line.size());
		size_t ind = opt.find("bm");

		std::cout << "[EPD, LINE " << std::setw(3) << lcnt << "]: " << line << '\n';

		if (ind != std::string::npos) {
			ind += 3;
			size_t last = nextToken(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(lcnt, equal, move, Search::_bestMove_unittest, search, pos, tmpgame, limits);
		}
		else {
			ind = opt.find("am");
			ASSERT(ind != std::string::npos, "Invalid line");
			ind += 3;
			size_t last = nextToken(opt, ind);
			move = Move32b::fromStr<Move32b::Notation::ALGEBRAIC>(pos, opt.substr(ind, last - ind));
			_TESTCASE(lcnt, nonequal, move, Search::_bestMove_unittest, search, pos, tmpgame, limits);
		}
	}

	time_ms_t duration_ms = timer.duration();

	std::cout << "TEST DURATION: " << duration_ms << "ms" << std::endl;
	return true;
}

static bool runTests(Search& search) {
	seeTests();
	ccrOneHourTest(search);
	return true;
}

} // namespace Units
