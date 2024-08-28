#include "UCI.hpp"
#include "..\backend\BitBoard.hpp"

int main(int argc, const char* argv[]) {
	UniversalChessInterface uci_obj;
	uci_obj.Loop(argc, argv);
}