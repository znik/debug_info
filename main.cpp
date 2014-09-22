#include <cstdio>
#include <string>
#include "varinfo.hpp"


int main(int argc, char *argv[]) {
	if (4 != argc) {
		printf("Usage: %s <bin_with_symbols> <var> <line>\n", argv[0]);
		return 0;
	}
	VarInfo vi;
	std::string prefix =
		//"";
		"/cs/systems/home/nzaborov/wiredtiger/build_posix/";
	if (!vi.init(argv[1], prefix)) {
		printf("Failed to initialize VarInfo.\n");
		return 0;
	}

	const std::string name = argv[2]; //"v1";
	int line = atoi(argv[3]);
	const std::string file =
		prefix + "../src/async/async_api.c";
		//"/cs/systems/home/nzaborov/debug_info/test.cpp";

	printf("Type of var \"%s\" at %s:%d is \"%s\"\n", name.c_str(),
		file.c_str(), line,
		vi.type(file, line, name).c_str()); 
	return 1;
}
