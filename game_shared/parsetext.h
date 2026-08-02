#pragma once
#ifndef PARSETEXT_H
#define PARSETEXT_H

#include "string_utils.h"
#include "template_property_types.h"
#include <cstddef>

void SkipSpaceCharacters(const char* text, int& i, const int length);
bool SkipSpacesAndTabs(const char* text, int& i, const int length);
void ConsumeNonSpaceCharacters(const char* text, int& i, const int length);
void ConsumeLine(const char* text, int& i, const int length);
void ConsumeLineSignificantOnly(const char* text, int& i, const int length);
bool ConsumeLineUntil(const char* text, int& i, const int length, char c);
bool ConsumePossiblyQuotedString(const char* text, int& i, const int length, int& strStart, int& strEnd);
bool ParseInteger(const char* valueText, int& result);
bool ParseColor(const char* valueText, int& result);
bool ParseBoolean(const char* valueText, bool& result);
bool ParseFloat(const char* valueText, float& result);
bool ParseFloatRange(const char* valueText, FloatRange& result);
bool ParseIntRange(const char* valueText, IntRange& result);

#endif
