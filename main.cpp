#include "C:\\Users\\Nik\\Documents\\debug_info\\varinfo.hpp"


int main(int argc, char *argv[]) {
	if (2 != argc) {
		printf("Usage: %s <bin_with_symbols>\n", argv[0]);
		return 0;
	}
	VarInfo vi;
	if (!vi.init(argv[1])) {
		printf("Failed to initialize VarInfo.\n");
		return 0;
	}
	return 1;
}