#pragma once
#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include "rapidjson/document.h"

class JSONConfig
{
public:
	bool ReadFromFile(const char* fileName);
	bool ReadFromContents(const char* contents, const char* fileName);
protected:
	virtual const char* Schema() const = 0;
	virtual bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) = 0;
};

#endif
