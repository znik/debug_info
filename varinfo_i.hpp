///
#pragma once

#include <string>


class IVarInfo {
public:
	virtual bool init(const std::string& file) = 0;
	virtual const std::string& type(const std::string& file, const size_t line, const std::string& name) const = 0;

protected:
	virtual ~IVarInfo() = 0;
};