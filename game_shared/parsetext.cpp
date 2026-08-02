#include "parsetext.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

void SkipSpaceCharacters(const char* text, int& i, const int length)
{
	while (i<length && IsSpaceCharacter(text[i]))
	{
		++i;
	}
}

bool SkipSpacesAndTabs(const char *text, int& i, const int length)
{
	int start = i;
	while (i<length && text[i] == ' ' || text[i] == '\t')
	{
		++i;
	}
	return i > start;
}

void ConsumeNonSpaceCharacters(const char *text, int& i, const int length)
{
	while(i<length && !IsSpaceCharacter(text[i]) && text[i] != '\0')
	{
		++i;
	}
}

void ConsumeLine(const char *text, int& i, const int length)
{
	while(i<length && text[i] != '\n' && text[i] != '\r' && text[i] != '\0')
	{
		++i;
	}
}

void ConsumeLineSignificantOnly(const char *text, int& i, const int length)
{
	const int origi = i;
	while(i<length && text[i] != '\n' && text[i] != '\r' && text[i] != '\0')
	{
		if (text[i] == '/' && i+1<length && text[i+1] == '/')
			break;
		++i;
	}
	if (origi != i)
	{
		int newi = i;
		while(text[newi-1] == ' ' && newi-1 >= origi)
		{
			newi--;
		}
		i = newi;
	}
}

bool ConsumeLineUntil(const char *text, int &i, const int length, char c)
{
	while(i<length && text[i] != c && text[i] != '\n' && text[i] != '\r' && text[i] != '\0')
	{
		++i;
	}
	return text[i] == c;
}

bool ConsumePossiblyQuotedString(const char* text, int& i, const int length, int& strStart, int& strEnd)
{
	const bool isQuoted = text[i] == '"';
	if (isQuoted)
	{
		++i;
		strStart = i;
		if (ConsumeLineUntil(text, i, length, '"')) {
			strEnd = i;
			++i;
		} else {
			return false;
		}
	}
	else
	{
		strStart = i;
		ConsumeNonSpaceCharacters(text, i, length);
		strEnd = i;
	}
	return strEnd - strStart > 0;
}

bool ParseInteger(const char *valueText, int& result)
{
	return sscanf(valueText, "%d", &result) == 1;
}

static int ClampColorComponent(int colorComponent)
{
	if (colorComponent > 255)
		return 255;
	else if (colorComponent < 0)
		return 0;
	else
		return colorComponent;
}

bool ParseColor(const char *valueText, int& result)
{
	if (strncmp(valueText, "0x", 2) == 0)
	{
		const char* colorStart = valueText + 2;
		return sscanf(colorStart, "%x", &result) == 1;
	}
	else if (*valueText == '#')
	{
		const char* colorStart = valueText + 1;
		return sscanf(colorStart, "%x", &result) == 1;
	}
	else
	{
		int r, g, b;
		if (sscanf(valueText, "%d %d %d", &r, &g, &b) == 3)
		{
			r = ClampColorComponent(r);
			g = ClampColorComponent(g);
			b = ClampColorComponent(b);
			result = (r << 16) + (g << 8) + b;
			return true;
		}
	}
	return false;
}

bool ParseBoolean(const char* valueText, bool& result)
{
	if (strcmp(valueText, "0") == 0 || stricmp(valueText, "false") == 0 || stricmp(valueText, "no") == 0)
	{
		result = false;
		return true;
	}
	else if (strcmp(valueText, "1") == 0 || stricmp(valueText, "true") == 0 || stricmp(valueText, "yes") == 0)
	{
		result = true;
		return true;
	}
	return false;
}

bool ParseFloat(const char *valueText, float& result)
{
	return sscanf(valueText, "%f", &result) == 1;
}

bool ParseFloatRange(const char *valueText, FloatRange& result)
{
	if (sscanf(valueText, "%f", &result.min) != 1)
		return false;

	const char* found = strchr(valueText, ',');

	if (found) {
		found++;
		if (sscanf(found, "%f", &result.max) != 1)
			return false;
	} else {
		result.max = result.min;
	}

	return true;
}

bool ParseIntRange(const char *valueText, IntRange& result)
{
	if (sscanf(valueText, "%d", &result.min) != 1)
		return false;

	const char* found = strchr(valueText, ',');

	if (found) {
		found++;
		if (sscanf(found, "%d", &result.max) != 1)
			return false;
	} else {
		result.max = result.min;
	}

	return true;
}
