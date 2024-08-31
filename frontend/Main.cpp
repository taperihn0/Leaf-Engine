#include "UCI.hpp"
#include "../backend/Magic.hpp"

/*
	NOTES:
	1. Dynamic bit shifting with relevant bits for every single square in 
	magic hashing might be simplified to shifting by 64 - 12 bits for rooks and 
	64 - 9 bits for bishops. 
*/

int main(int argc, const char* argv[]) {
	UniversalChessInterface uci_obj;
	uci_obj.loop(argc, argv);
}