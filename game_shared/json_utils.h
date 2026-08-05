#pragma once
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <cstdint>
#include <string>
#include <type_traits>

#include "fixed_string.h"
#include "optional.h"
#include "tribool.h"
#include "vector.h"
#include "damageinfo.h"
#include "rapidjson/document.h"
#include "template_property_types.h"
#include "skillbasedvalue.h"

bool ReadJsonDocumentWithSchema(rapidjson::Document& document, const char* pMemFile, int fileSize, const char* schemaText, const char* fileName);
bool ReadJsonDocumentWithSchemaFromFile(rapidjson::Document& document, const char* fileName, const char* schemaText);

template<typename F, typename = std::enable_if<
	std::is_convertible<
		decltype(std::declval<F>()(std::declval<const rapidjson::Value&>())), void
		>::value>
>
void HandleJSONMember(const rapidjson::Value& value, const char* name, F f)
{
	auto it = value.FindMember(name);
	if (it != value.MemberEnd())
	{
		f(it->value);
	}
}

template<typename F, typename = std::enable_if<
	std::is_convertible<
		decltype(std::declval<F>()(std::declval<const char*>())), int
		>::value>
>
int JSONStringSetToFlags(const rapidjson::Value& value, F f)
{
	int result{};
	if (value.IsArray())
	{
		rapidjson::Value::ConstArray arr = value.GetArray();
		for (auto& item : arr)
		{
			result |= f(item.GetString());
		}
	}
	else
	{
		result = f(value.GetString());
	}
	return result;
}

optional<FloatRange> FloatRangeFromJSON(const rapidjson::Value& jsonValue);
optional<IntRange> IntRangeFromJSON(const rapidjson::Value& jsonValue);

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
bool UpdatePropertyFromJson(short& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(int& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(unsigned int& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(std::int64_t& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(std::uint64_t& i, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(float& f, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(bool& b, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(char& c, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Color3& color, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(FloatRange& floatRange, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(IntRange& intRange, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Vector& vector, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(tribool& b, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(SkillBasedValue& skillValue, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(DamageInfoPatch& damageInfo, const rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(RadiusDamageInfoPatch& damageInfo, const rapidjson::Value& jsonValue, const char* key);

template<typename T>
bool UpdatePropertyFromJson(optional<T>& v, const rapidjson::Value& jsonValue, const char* key)
{
	T val;
	if (UpdatePropertyFromJson(val, jsonValue, key))
	{
		v = val;
		return true;
	}
	return false;
}

bool UpdateAttenuationFromJson(float& attn, const rapidjson::Value& jsonValue);
void UpdatePlayerShake(PlayerShake& shake, const rapidjson::Value& value);

int DamageTypeFromJSON(const rapidjson::Value& value);
bool UpdateDamageInfoFromJson(DamageInfoPatch &damageInfo, const rapidjson::Value &value);

#endif
