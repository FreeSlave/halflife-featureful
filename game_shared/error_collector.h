#pragma once
#ifndef ERROR_COLLECTOR_H
#define ERROR_COLLECTOR_H

#include <vector>
#include <string>

class ErrorCollector
{
public:
	void AddError(const char* str);
	void AddFormattedError(const char* format, ...);
	void AddDeprecation(const char* str);
	void AddFormattedDeprecation(const char* format, ...);
	bool HasErrors() const;
	std::string GetFullString() const;
	void Clear();
	void ClearDeprecations();

	std::vector<std::string>::const_iterator DeprecationsBegin() const {
		return _deprecations.begin();
	}
	std::vector<std::string>::const_iterator DeprecationsEnd() const {
		return _deprecations.end();
	}
private:
	std::vector<std::string> _errors;
	std::vector<std::string> _deprecations;
};

extern ErrorCollector g_errorCollector;

#endif
