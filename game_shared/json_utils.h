#pragma once
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>

#include "vector.h"
#include "rapidjson/document.h"
#include "template_property_types.h"

bool ReadJsonDocumentWithSchema(rapidjson::Document& document, const char* pMemFile, int fileSize, const char* schemaText, const char* fileName);
bool ReadJsonDocumentWithSchemaFromFile(rapidjson::Document& document, const char* fileName, const char* schemaText);

bool UpdatePropertyFromJson(std::string& str, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(int& i, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(float& f, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(bool& b, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(char& c, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Color3& color, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(FloatRange& floatRange, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(IntRange& intRange, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Vector& vector, rapidjson::Value& jsonValue, const char* key);
bool UpdateAttenuationFromJson(float& attn, rapidjson::Value& jsonValue);

#endif
