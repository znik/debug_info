#include <cstdio>
#include <string>
#include "varinfo.hpp"


int main(int argc, char *argv[]) {
	if (3 != argc) {
		printf("Usage: %s <bin_with_symbols> <var>\n", argv[0]);
		return 0;
	}
	VarInfo vi;
	if (!vi.init(argv[1])) {
		printf("Failed to initialize VarInfo.\n");
		return 0;
	}

	const std::string name = argv[2]; //"v1";
	int line = 11;
	const std::string file =
		"/cs/systems/home/nzaborov/debug_info/test.cpp";
	
	printf("Type of var \"%s\" at %s:%d is \"%s\"\n", name.c_str(),
		file.c_str(), line,
		vi.type(file, line, name).c_str()); 
	return 1;
}
