///
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "scoping.h"


bool scoping::init(const std::vector<std::string>& srcfiles) {
	for (const std::string& f : srcfiles) {
		std::ifstream fstream;
		fstream.open(f.c_str());
		if (!fstream.is_open()) {
			assert(false && "Scoping: cannot open file");
			printf("Scoping: cannot open file %s\n", f.c_str());
			return false;
		}
		int nesting_level = 0;
		int lineno = 0;
		struct tri_t {
			static tri_t make(int level, int start, int end) { return tri_t(level, start, end); }
			int _level, _start, _end;
		private:
			tri_t(int level, int start, int end) : _level(level), _start(start), _end(end) {};
		};
		std::vector<tri_t> scopes;
		struct look_for_empty_end_of_nesting_level {
			look_for_empty_end_of_nesting_level(int level) : _level(level) {};
			bool operator() (const tri_t &item) { return item._level == _level && NO_END_LINE == item._end; }
		private:
			const int _level;
		};

		scopes.push_back(tri_t::make(nesting_level, 1, NO_END_LINE));
		++nesting_level;
		std::string line;

		while(std::getline(fstream, line)) {
			++lineno;
			for (unsigned i = 0; i < line.size(); ++i) {
				if ('{' == line[i]) {
					scopes.push_back(tri_t::make(nesting_level, lineno, NO_END_LINE));
					++nesting_level;
				}
				else if ('}' == line[i]) {
					--nesting_level;
					auto item = std::find_if(scopes.begin(), scopes.end(), look_for_empty_end_of_nesting_level(nesting_level));
					if (scopes.end() == item) {
						printf("Closing bracked without opening one in line %d\n", lineno);
						assert(false && "Closing bracket without opening bracket");
						return false;
					}
					item->_end = lineno;
				}
			}
		}
		assert(1 == nesting_level && "Not balanced brackets");
		--nesting_level;
		scopes[nesting_level]._end = lineno;
		fstream.close();

		for (auto i : scopes) {
			_scopes[f][i._start] = i._end;
		}
	}
	return true;
}
