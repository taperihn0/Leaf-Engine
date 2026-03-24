#include "UtilsProtocol.hpp"
#include "frontend/Setup.hpp"

int main(int argc, const char* argv[]) {
	setupInternals();
	Utils::UtilsProtocol protocol;
	protocol.loop(argc, argv);
}
