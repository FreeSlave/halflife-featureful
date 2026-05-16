#include "file_utils.h"
#include "json_utils.h"
#include "logger.h"
#include "const_sound.h"
#include "safe_snprintf.h"

#include "color_utils.h"
#include "parsetext.h"
#include "error_collector.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"

#include "json_schemas.h"

using namespace rapidjson;

static void CalculateLineAndColumnFromOffset(const char* pMemFile, size_t offset, size_t& line, size_t& column)
{
	const char* cur = pMemFile;
	line = 1;
	column = 0;
	size_t i = 0;
	bool nextLine = false;
	while (cur && *cur && i <= offset)
	{
		if (nextLine)
		{
			nextLine = false;
			line++;
			column = 0;
		}
		if (*cur == '\n')
		{
			nextLine = true;
		}
		++column;
		++cur;
		++i;
	}
}

static void ReportParseErrors(const char* fileName, ParseResult& parseResult, const char *pMemFile)
{
	size_t errorLine, errorColumn;
	CalculateLineAndColumnFromOffset(pMemFile, parseResult.Offset(), errorLine, errorColumn);
	g_errorCollector.AddFormattedError("%s: JSON parse error: %s (Line %zu, column %zu)", fileName, GetParseError_En(parseResult.Code()), errorLine, errorColumn);
}

class GeneralDefinitionsProvider : public IRemoteSchemaDocumentProvider
{
public:
	GeneralDefinitionsProvider(const Document& schemaDocument): _schema(schemaDocument) {}
	const SchemaDocument* GetRemoteDocument(const char* uri, SizeType length) {
		return &_schema;
	}
private:
	SchemaDocument _schema;
};

class DefinitionsProvider : public IRemoteSchemaDocumentProvider
{
public:
	DefinitionsProvider(const Document& schemaDocument, const Document& weaponSchemaDocument): _generalProvider(schemaDocument), _weaponSchema(weaponSchemaDocument, 0, 0, &_generalProvider) {}
	const SchemaDocument* GetRemoteDocument(const char* uri, SizeType length) {
		if (uri && *uri && strncmp(uri, "weapons.json", length) == 0)
		{
			return &_weaponSchema;
		}
		return _generalProvider.GetRemoteDocument(uri, length);
	}
private:
	GeneralDefinitionsProvider _generalProvider;
	SchemaDocument _weaponSchema;
};

IRemoteSchemaDocumentProvider* GetDefinitionsProvider()
{
	static std::unique_ptr<DefinitionsProvider> provider;
	static bool tried = false;

	if (tried)
		return provider.get();

	tried = true;

	if (!provider)
	{
		Document definitionsSchemaDocument;
		definitionsSchemaDocument.Parse(json_schemas::definitions);
		ParseResult parseResult = definitionsSchemaDocument;
		if (!parseResult)
		{
			ReportParseErrors("definitions", parseResult, json_schemas::definitions);
			return nullptr;
		}

		Document weaponsSchemaDocument;
		weaponsSchemaDocument.Parse(json_schemas::weapons);
		parseResult = weaponsSchemaDocument;

		if (!parseResult)
		{
			ReportParseErrors("weapons", parseResult, json_schemas::weapons);
			return nullptr;
		}

		provider = std::unique_ptr<DefinitionsProvider>(new DefinitionsProvider(definitionsSchemaDocument, weaponsSchemaDocument));
	}
	return provider.get();
}

bool ReadJsonDocumentWithSchema(Document &document, const char *pMemFile, int fileSize, const char *schemaText, const char* fileName)
{
	if (!fileName)
		fileName = "";

	Document schemaDocument;
	schemaDocument.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(schemaText);
	ParseResult parseResult = schemaDocument;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	IRemoteSchemaDocumentProvider* provider = GetDefinitionsProvider();
	if (!provider)
		return false;
	SchemaDocument schema(schemaDocument, 0, 0, provider);

	document.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(pMemFile, fileSize);
	parseResult = document;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	SchemaValidator validator(schema);
	if (!document.Accept(validator))
	{
		Pointer schemaPointer = validator.GetInvalidSchemaPointer();
		StringBuffer schemaPathBuffer;
		schemaPointer.Stringify(schemaPathBuffer);

		Pointer docPointer = validator.GetInvalidDocumentPointer();
		StringBuffer docPathBuffer;
		docPointer.Stringify(docPathBuffer);

		const char* invalidKeyword = validator.GetInvalidSchemaKeyword();
		auto& errorVal = validator.GetError();

		char buf[1028];
		if (strcmp(invalidKeyword, "additionalProperties") == 0)
		{
			safe_snprintf(buf, sizeof(buf), "%s: unknown property \"%s\" is prohibited\n", fileName, docPathBuffer.GetString());
		}
		else if (strcmp(invalidKeyword, "dependencies") == 0 && errorVal.HasMember(invalidKeyword) && errorVal[invalidKeyword].HasMember("errors"))
		{
			const char* dependentPropertyName = "";

			auto& errors = errorVal[invalidKeyword]["errors"];
			auto it = errors.MemberBegin();
			if (it != errors.MemberEnd())
			{
				dependentPropertyName = it->name.GetString();
			}

			safe_snprintf(buf, sizeof(buf), "%s: dependencies constraint is unmet (incompatible properties are given) in \"%s\": '%s'\n", fileName, docPathBuffer.GetString(), dependentPropertyName);
		}
		else
		{
			StringBuffer badValueBuffer;
			Value *badVal = GetValueByPointer(document, docPointer);
			if (badVal)
			{
				Writer<StringBuffer> writer(badValueBuffer);
				badVal->Accept(writer);
			}

			StringBuffer schemaPartBuffer;
			if (errorVal.HasMember(invalidKeyword))
			{
				Writer<StringBuffer> writer(schemaPartBuffer);
				errorVal[invalidKeyword].Accept(writer);
			}

			safe_snprintf(buf, sizeof(buf), "%s: property \"%s\" : %s doesn't match the constraint '%s' in '%s': %s\n",
				fileName,
				docPathBuffer.GetString(),
				badValueBuffer.GetString(),
				invalidKeyword,
				schemaPathBuffer.GetString(),
				schemaPartBuffer.GetString());
		}

		g_errorCollector.AddError(buf);

		return false;
	}
	return true;
}

bool ReadJsonDocumentWithSchemaFromFile(Document &document, const char *fileName, const char *schemaText)
{
	int fileSize;
	char *pMemFile = nullptr;
	pMemFile = ReadFileContents(fileName, fileSize);
	if (!pMemFile)
		return false;

	LOG("Parsing %s\n", fileName);

	const bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, schemaText, fileName);
	FreeFileContents(pMemFile);
	return success;
}

bool UpdatePropertyFromJson(std::string& str, const Value& jsonValue, const char* key)
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

bool UpdatePropertyFromJson(short& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = static_cast<short>(it->value.GetInt());
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(int& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(unsigned int& i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetUint();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(std::int64_t & i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt64();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(std::uint64_t & i, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetUint64();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(float& f, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		f = it->value.GetFloat();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(bool& b, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(char& c, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.GetStringLength() == 1)
		{
			c = *it->value.GetString();
			return true;
		}
	}
	return false;
}

bool UpdatePropertyFromJson(Color3& color, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.IsString())
		{
			const char* colorStr = it->value.GetString();
			int packedColor;
			if (ParseColor(colorStr, packedColor)) {
				UnpackRGB(color.r, color.g, color.b, packedColor);
				return true;
			}
		}
		else if (it->value.IsArray())
		{
			Value::ConstArray arr = it->value.GetArray();
			color.r = arr[0].GetInt();
			color.g = arr[1].GetInt();
			color.b = arr[2].GetInt();
			return true;
		}
	}
	return false;
}

optional<FloatRange> FloatRangeFromJSON(const Value& value)
{
	auto fixupRange = [](FloatRange& floatRange)
	{
		if (floatRange.min > floatRange.max) {
			floatRange.min = floatRange.max;
		}
		return floatRange;
	};

	optional<FloatRange> emptyResult;
	FloatRange floatRange;
	if (value.IsNumber())
	{
		floatRange.min = value.GetFloat();
		floatRange.max = floatRange.min;
		return optional<FloatRange>(floatRange);
	}
	else if (value.IsObject())
	{
		if (value.MemberCount() == 2)
		{
			auto minIt = value.FindMember("min");
			if (minIt == value.MemberEnd())
				return emptyResult;
			auto maxIt = value.FindMember("max");
			if (maxIt == value.MemberEnd())
				return emptyResult;

			if (!minIt->value.IsNumber() || !maxIt->value.IsNumber())
				return emptyResult;

			floatRange.min = minIt->value.GetFloat();
			floatRange.max = maxIt->value.GetFloat();
			return optional<FloatRange>(fixupRange(floatRange));
		}
	}
	else if (value.IsString())
	{
		if (ParseFloatRange(value.GetString(), floatRange))
		{
			return optional<FloatRange>(fixupRange(floatRange));
		}
		else
		{
			return emptyResult;
		}
	}
	else if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		if (arr.Size() == 2 && arr[0].IsNumber() && arr[1].IsNumber())
		{
			floatRange.min = arr[0].GetFloat();
			floatRange.max = arr[1].GetFloat();
			return optional<FloatRange>(fixupRange(floatRange));
		}
	}
	return emptyResult;
}

optional<IntRange> IntRangeFromJSON(const rapidjson::Value& value)
{
	auto fixupRange = [](IntRange& intRange)
	{
		if (intRange.min > intRange.max) {
			intRange.min = intRange.max;
		}
		return intRange;
	};

	optional<IntRange> emptyResult;
	IntRange intRange;
	if (value.IsInt())
	{
		intRange.min = value.GetInt();
		intRange.max = intRange.min;
		return optional<IntRange>(intRange);
	}
	else if (value.IsObject())
	{
		if (value.MemberCount() == 2)
		{
			auto minIt = value.FindMember("min");
			if (minIt == value.MemberEnd())
				return emptyResult;
			auto maxIt = value.FindMember("max");
			if (maxIt == value.MemberEnd())
				return emptyResult;

			if (!minIt->value.IsInt() || !maxIt->value.IsInt())
				return emptyResult;

			intRange.min = minIt->value.GetInt();
			intRange.max = maxIt->value.GetInt();
			return optional<IntRange>(fixupRange(intRange));
		}
	}
	else if (value.IsString())
	{
		if (ParseIntRange(value.GetString(), intRange))
		{
			return optional<IntRange>(fixupRange(intRange));
		}
		else
		{
			return emptyResult;
		}
	}
	else if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		if (arr.Size() == 2 && arr[0].IsInt() && arr[1].IsInt())
		{
			intRange.min = arr[0].GetInt();
			intRange.max = arr[1].GetInt();
			return optional<IntRange>(fixupRange(intRange));
		}
	}
	return emptyResult;
}

bool UpdatePropertyFromJson(FloatRange& floatRange, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		optional<FloatRange> range = FloatRangeFromJSON(it->value);
		if (range.has_value())
		{
			floatRange = *range;
			return true;
		}
	}
	return false;
}

bool UpdatePropertyFromJson(IntRange& intRange, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		optional<IntRange> range = IntRangeFromJSON(it->value);
		if (range.has_value())
		{
			intRange = *range;
			return true;
		}
	}
	return false;
}

bool UpdatePropertyFromJson(Vector& vector, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value::ConstArray arr = it->value.GetArray();
		vector.x = arr[0].GetFloat();
		vector.y = arr[1].GetFloat();
		vector.z = arr[2].GetFloat();

		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(tribool& b, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

SkillBasedValue SkillBasedValueFromJSON(const Value& value)
{
	SkillBasedValue skillValue;
	auto floatRange = FloatRangeFromJSON(value);
	if (floatRange.has_value())
	{
		skillValue.easy = skillValue.medium = skillValue.hard = *floatRange;
		skillValue.type = SkillBasedValue::COMMON;
		return skillValue;
	}
	else if (value.IsString())
	{
		skillValue.skillVariable = value.GetString();
		skillValue.type = SkillBasedValue::STRING;
		return skillValue;
	}
	else if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		if (arr.Size() == 3)
		{
			skillValue.type = SkillBasedValue::DIFFICULTIES;
			skillValue.easy = FloatRangeFromJSON(arr[0]).value_or(FloatRange());
			skillValue.medium = FloatRangeFromJSON(arr[1]).value_or(FloatRange());
			skillValue.hard = FloatRangeFromJSON(arr[2]).value_or(FloatRange());
			return skillValue;
		}
	}
	return skillValue;
}

bool UpdatePropertyFromJson(SkillBasedValue& skillValue, const Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		SkillBasedValue skillBasedValue = SkillBasedValueFromJSON(it->value);
		if (skillBasedValue.IsDefined())
		{
			skillValue = std::move(skillBasedValue);
			return true;
		}
	}
	return false;
}

static bool ParseAttenuation(const char* str, float& attenuation)
{
	constexpr std::pair<const char*, float> attenuations[] = {
		{"norm", ATTN_NORM},
		{"idle", ATTN_IDLE},
		{"static", ATTN_STATIC},
		{"none", ATTN_NONE},
	};

	for (auto& p : attenuations)
	{
		if (stricmp(str, p.first) == 0)
		{
			attenuation = p.second;
			return true;
		}
	}
	return false;
}

bool UpdateAttenuationFromJson(float& attn, const Value& jsonValue)
{
	if (jsonValue.IsString())
	{
		return ParseAttenuation(jsonValue.GetString(), attn);
	}
	else if (jsonValue.IsNumber())
	{
		attn = jsonValue.GetFloat();
		return true;
	}
	return false;
}

void UpdatePlayerShake(PlayerShake& shake, const Value& value)
{
	if (value.IsNull())
	{
		shake = PlayerShake();
	}
	else if (value.IsObject())
	{
		UpdatePropertyFromJson(shake.radius, value, "radius");
		UpdatePropertyFromJson(shake.amplitude, value, "amplitude");
		UpdatePropertyFromJson(shake.duration, value, "duration");
		UpdatePropertyFromJson(shake.frequency, value, "frequency");
	}
}

int DamageTypeFromJSON(const Value& value)
{
	return JSONStringSetToFlags(value, [](const char* damageTypeName) {
		int subType = ParseDamageType(damageTypeName);
		if (subType >= 0)
		{
			return subType;
		}
		else
		{
			LOG_WARNING("Unknown damage type '%s'\n", damageTypeName);
			return 0;
		}
	});
}

bool UpdateDamageInfoFromJson(DamageInfoPatch &damageInfo, const rapidjson::Value &value)
{
	if (!value.IsObject())
		return false;

	UpdatePropertyFromJson(damageInfo.damage, value, "damage");

	HandleJSONMember(value, "type", [&damageInfo](const Value& value) {
		damageInfo.type = DamageTypeFromJSON(value);
	});

	HandleJSONMember(value, "type_policy", [&damageInfo](const Value& value) {
		const char* typePolicyName = value.GetString();
		if (strcmp(typePolicyName, "add") == 0)
		{
			damageInfo.typePolicy = DamageInfoPatch::ADD_DAMAGE_TYPE;
		}
		else if (strcmp(typePolicyName, "replace") == 0)
		{
			damageInfo.typePolicy = DamageInfoPatch::REPLACE_DAMAGE_TYPE;
		}
	});

	UpdatePropertyFromJson(damageInfo.nonLethal, value, "nonlethal");
	UpdatePropertyFromJson(damageInfo.ignoreArmor, value, "ignore_armor");
	UpdatePropertyFromJson(damageInfo.noBlood, value, "no_blood");

	HandleJSONMember(value, "gib", [&](const Value& value) {
		damageInfo.gibPolicy = ParseGibPolicy(value.GetString());
	});

	return true;
}
