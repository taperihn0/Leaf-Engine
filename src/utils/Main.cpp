#include "UtilsProtocol.hpp"
#include "Magic.hpp"

int main(int argc, const char* argv[]) {
	SlidersAttacks::initTables();
	Utils::UtilsProtocol protocol;
	protocol.loop(argc, argv);
}
