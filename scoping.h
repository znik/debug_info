#pragma once
#include <map>
#include <string>
#include <vector>
#include <cassert>


struct scoping {
	enum {NO_END_LINE = -1};
	bool init(const std::vector<std::string>& /*srcfiles*/);
	inline int endline(const std::string& file, int startline) const {
		assert(scope_t() != _scopes.at(file) && "Scoping: no file");
		if (0 == _scopes.at(file).at(startline))
			return NO_END_LINE;
		return _scopes.at(file).at(startline);
	}
private:
	typedef std::map<int, int> scope_t;
	std::map<std::string, scope_t> _scopes;
};
