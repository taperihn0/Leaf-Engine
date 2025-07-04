#pragma once

#include "../backend/Common.hpp"
#include "../backend/Position.hpp"
#include "../backend/Move.hpp"

#include <iomanip>
#include <fstream>

#define _TESTCASE(expc, f, ...)													  \
{																				  \
	_testcase_assertion(f, expc, #f "(" #__VA_ARGS__ ")", __LINE__, __VA_ARGS__); \
}																				  \

static size_t test_counter = 0;

template <typename Func, typename T, typename... Args>
bool _testcase_assertion(Func f, T expected, std::string_view fstr, int line, Args&&... args) {
	T fres = f(std::forward<Args>(args)...);
	bool succes = fres == expected;
	std::cout << "[n: " << std::setw(3) << test_counter << ", line: " << std::setw(3) << line << "] $ Testcase "
		<< (succes ? "passed" : "failed") << ": " << fstr
		<< (succes ? " == " : " != ") << expected << std::endl;
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

		if (move.isQuiet())
			continue;

		_TESTCASE(expected, StaticExchangeEval_3a, pos, move.getOrigin(), move.getTarget());
	}

	return true;
}

static bool runTests() {
	seeTests();
	return true;
}

} // namespace Units