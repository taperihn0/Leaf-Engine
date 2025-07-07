#pragma once

#include "../backend/Common.hpp"
#include "../backend/Position.hpp"
#include "../backend/Move.hpp"

#include <iomanip>
#include <fstream>

#define _COLOR_RED		  "\033[0;31m"
#define _COLOR_BRIGHT_RED "\033[0;91m"
#define _COLOR_GREEN	  "\033[0;32m"
#define _COLOR_RESET	  "\033[0m"

#define _TESTCASE(lcnt, cmp, expc, f, ...)																\
{																										\
	_testcase_assertion(f, expc, cmp, #cmp, #f "(" #__VA_ARGS__ ")", lcnt, (int)__LINE__, __VA_ARGS__); \
}																										\

static size_t test_counter = 0;

template <typename T>
bool equal(const T& a, const T& b) {
	return a == b;
}

template <typename T>
bool samesign(const T& a, const T& b) {
	static_assert(std::is_integral<T>::value, "samesign handles only integer types");
	return (a < 0ull and b < 0ull) or (a > 0ull and b > 0ull) 
		   or (a == 0ull and b == 0ull);
}

template <typename T>
using _cmp_func_t = bool(*)(const T&, const T&);

template <typename Func, typename T, typename... Args>
bool _testcase_assertion(Func f, T expected, _cmp_func_t<T> cmp, std::string_view cmpnamestr, 
	std::string_view fcallstr, int testline, int fileline, Args&&... args) {
	T fres = f(std::forward<Args>(args)...);
	bool succes = cmp(fres, expected);
	if (!succes) std::cout << _COLOR_RED;
	std::cout << "["
		<< "TESTNUM: " << std::setw(3) << test_counter 
		<< ", TESTLINE: " << std::setw(3) << testline 
		<< ", FILELINE: " << std::setw(3) << fileline 
		<< "] $ "
		<< (succes ? _COLOR_GREEN "TESTCASE PASSED" _COLOR_RESET : _COLOR_BRIGHT_RED "TESTCASE FAILED" _COLOR_RED)
		<< ": " << fcallstr
		<< ", " << cmpnamestr << "(" << fres << ", " << expected << ") = " << (succes ? "TRUE" : "FALSE")
		<< _COLOR_RESET << std::endl;
	test_counter++;
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

	for (int lcnt = 0; std::getline(file, line); lcnt++) {
		size_t ind = 0;
		Move move = Move::null;
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
				move = Move::fromStr<Move::Notation::ALGEBRAIC>(pos, token);
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
		 const Piece::enumType piece = move.getPerformerT();
		 const Piece::enumType target = pos.pieceTypeOn(dst, pos.getOppositeTurn());

		_TESTCASE(lcnt, equal, expected, StaticExchangeEval_3a<true>, pos, move.getOrigin(), 
				  dst, target, piece);
		_TESTCASE(lcnt, samesign, expected, StaticExchangeEval_3a<false>, pos, move.getOrigin(), 
				  dst, target, piece);

	}

	return true;
}

static bool runTests() {
	seeTests();
	return true;
}

} // namespace Units