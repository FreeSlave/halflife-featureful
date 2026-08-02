#include "error_collector.h"
#include "logger.h"
#include <cstdarg>
#include <cstdio>

void ErrorCollector::AddError(const char *str)
{
	if (str)
	{
		_errors.push_back(str);
		LOG_ERROR("%s\n", str);
	}
}

void ErrorCollector::AddFormattedError(const char *format, ...)
{
	va_list	argptr;
	static char buf[1024];

	va_start(argptr, format);
	_vsnprintf(buf, sizeof(buf), format, argptr);
	va_end(argptr);

	AddError(buf);
}

void ErrorCollector::AddDeprecation(const char *str)
{
	if (str)
	{
		_deprecations.push_back(str);
		LOG("DEPRECATION: %s\n", str);
	}
}

void ErrorCollector::AddFormattedDeprecation(const char *format, ...)
{
	va_list	argptr;
	static char buf[1024];

	va_start(argptr, format);
	_vsnprintf(buf, sizeof(buf), format, argptr);
	va_end(argptr);

	AddDeprecation(buf);
}

bool ErrorCollector::HasErrors() const
{
	return _errors.size() > 0;
}

std::string ErrorCollector::GetFullString() const
{
	std::string fullString;
	for (const std::string& str : _errors)
	{
		fullString += str;
		fullString += '\n';
	}
	return fullString;
}

void ErrorCollector::Clear()
{
	_errors.clear();
}

void ErrorCollector::ClearDeprecations()
{
	_deprecations.clear();
}

ErrorCollector g_errorCollector;
