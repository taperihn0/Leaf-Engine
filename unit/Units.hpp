#pragma once

#include "../backend/Common.hpp"
#include "../backend/Position.hpp"
#include "../backend/Move.hpp"

#include <iomanip>
#include <fstream>

#define _COLOR_RED	 "\033[0;31m"
#define _COLOR_GREEN "\033[0;32m"
#define _COLOR_RESET "\033[0m"

#define _TESTCASE(expc, f, ...)													  \
{																				  \
	_testcase_assertion(f, expc, #f "(" #__VA_ARGS__ ")", __LINE__, __VA_ARGS__); \
}																				  \

static size_t test_counter = 0;

template <typename Func, typename T, typename... Args>
bool _testcase_assertion(Func f, T expected, std::string_view fstr, int line, Args&&... args) {
	T fres = f(std::forward<Args>(args)...);
	bool succes = fres == expected;
	std::cout << "[NUM: " << std::setw(3) << test_counter << ", LINE: " << std::setw(3) << line << "] $ "
		<< (succes ? _COLOR_GREEN "TESTCASE PASSED" : _COLOR_RED "TESTCASE FAILED") << _COLOR_RESET ": " << fstr
		<< ", " << fres << (succes ? " == " : " != ") << expected << std::endl;
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

	while (std::getline(file, line)) {
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

		std::cout << "[EPD]: " << line << '\n';
		_TESTCASE(expected, StaticExchangeEval_3a, pos, move.getOrigin(), move.getTarget());
	}

	return true;
}

static bool runTests() {
	seeTests();
	return true;
}

} // namespace Units