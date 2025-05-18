#pragma once
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <type_traits>

#include "fixed_string.h"
#include "optional.h"
#include "tribool.h"
#include "vector.h"
#include "rapidjson/document.h"
#include "template_property_types.h"

bool ReadJsonDocumentWithSchema(rapidjson::Document& document, const char* pMemFile, int fileSize, const char* schemaText, const char* fileName);
bool ReadJsonDocumentWithSchemaFromFile(rapidjson::Document& document, const char* fileName, const char* schemaText);

template<typename F, typename = std::enable_if<
						 std::is_convertible<
							 decltype(std::declval<F>()(std::declval<const rapidjson::Value&>())), void
							 >::value
						 >
		 >
void HandleJSONMember(const rapidjson::Value& value, const char* name, F f)
{
	auto it = value.FindMember(name);
	if (it != value.MemberEnd())
	{
		f(it->value);
	}
}

template<size_t N>
bool UpdatePropertyFromJson(fixed_string<N>& str, const rapidjson::Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.IsNull())
		{
			str.clear();
			return true;
		}
		else if (it->value.IsString())
		{
			str = it->value.GetString();
			return true;
		}
		else
			return false;
	}
	return false;
}
bool UpdatePropertyFromJson(std::string& str, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(int& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(float& f, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(optional<float>& f, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(optional<int>& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(bool& b, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(char& c, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Color3& color, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(FloatRange& floatRange, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(IntRange& intRange, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Vector& vector, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(tribool& b, const rapidjson::Value& jsonValue, const char* key);
bool UpdateAttenuationFromJson(float& attn, const rapidjson::Value& jsonValue);

#endif
